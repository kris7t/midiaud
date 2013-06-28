
#include "jack_midi_player.h"

#include <stdexcept>
#include <iostream>

namespace midiaud {

JackMidiPlayer::JackMidiPlayer(std::string client_name,
                               std::string port_name)
    : client_name_(client_name), port_name_(port_name), activated_(false),
      timebase_master_(false), keep_running_(true) {
  jack_client_ = jack_client_open(client_name_.c_str(),
                                  JackNullOption, nullptr);
  if (jack_client_ == nullptr)
    throw std::runtime_error("jack_client_open failed");
  if (jack_set_sync_callback(
          jack_client_, &JackMidiPlayer::StaticSyncCallback, this) != 0)
    throw std::runtime_error("jack_set_sync_callback failed");
  if (jack_set_process_callback(
          jack_client_, &JackMidiPlayer::StaticProcessCallback, this) != 0)
    throw std::runtime_error("jack_set_process_callback failed");
  jack_on_shutdown(jack_client_,
                   &JackMidiPlayer::StaticShutdownCallback, this);
  midi_port_ = jack_port_register(jack_client_, port_name_.c_str(),
                                  JACK_DEFAULT_MIDI_TYPE,
                                  JackPortIsOutput, 0);
  if (midi_port_ == nullptr)
    throw std::runtime_error("jack_port_register failed");
}

JackMidiPlayer::~JackMidiPlayer() {
  if (jack_client_ != nullptr) {
    if (activated_) jack_deactivate(jack_client_);
    if (midi_port_ != nullptr)
      jack_port_unregister(jack_client_, midi_port_);
    jack_client_close(jack_client_);
  }
}

void JackMidiPlayer::Activate() {
  activated_ = true;
  keep_running_.store(true, std::memory_order_relaxed);
  if (jack_activate(jack_client_) != 0)
    throw std::runtime_error("jack_activate failed");
}

void JackMidiPlayer::Deactivate() {
  if (!activated_) return;
  keep_running_.store(false, std::memory_order_relaxed);

  if (jack_deactivate(jack_client_) == 0) {
    // If there was already a pending exception and thus we don't
    // throw because of the deactivation failure, activated() will
    // still signal about it.
    activated_ = false;
  } else if (!pending_exception_) {
    // If an exception was already thrown in the RT thread,
    // jack_deactivate might also potentially fail (e.g. server was
    // shut down), however, the first exception is more important to
    // be thrown.
    throw std::runtime_error("jack_deactivate failed");
  }

  if (pending_exception_) {
    auto the_exception = pending_exception_;
    pending_exception_ = nullptr;
    std::rethrow_exception(the_exception);
  }
}

void JackMidiPlayer::SetTimebaseMaster(bool conditional) {
  if (timebase_master_) return;
  std::cerr << "Acquiring timebase master" << std::endl;
  if (jack_set_timebase_callback(jack_client_, conditional,
                                 &JackMidiPlayer::StaticTimebaseCallback,
                                 this) != 0)
    throw std::runtime_error("jack_set_timebase_callback failed");
  timebase_master_ = true;
}

void JackMidiPlayer::ReleaseTimebaseMaster() {
  if (!timebase_master_) return;
  std::cerr << "Releasing timebase master" << std::endl;
  if (jack_release_timebase(jack_client_) != 0)
    throw std::runtime_error("jack_release_timebase failed");
  timebase_master_ = false;
}

int JackMidiPlayer::SyncCallback(jack_transport_state_t state,
                                 jack_position_t *pos) {
  if (state == JackTransportStarting) {
    SmfStreamer *smf_streamer = smf_streamer_container_.Fetch();
    double pos_seconds = static_cast<double>(pos->frame)
        / pos->frame_rate;
    smf_streamer->Reposition(pos_seconds);
  }
  return true;
}

int JackMidiPlayer::ProcessCallback(jack_nframes_t nframes) {
  jack_position_t pos;
  jack_transport_state_t state = jack_transport_query(
      jack_client_, &pos);

  JackMidiSink midi_sink(midi_port_, nframes, pos.frame_rate);

  bool now_playing = (state == JackTransportRolling);
  double start_seconds = static_cast<double>(pos.frame)
      / pos.frame_rate;
  double end_seconds = static_cast<double>(pos.frame + nframes)
      / pos.frame_rate;

  SmfStreamer *smf_streamer = smf_streamer_container_.Fetch();
  if (!smf_streamer->initialized())
    smf_streamer->Reposition(start_seconds);
  smf_streamer->StopIfNeeded(now_playing, midi_sink);
  if (now_playing) smf_streamer->CopyToSink(start_seconds,
                                            end_seconds,
                                            midi_sink);
  return 0;
}

void JackMidiPlayer::TimebaseCallback(jack_transport_state_t state,
                                      jack_nframes_t nframes,
                                      jack_position_t *pos,
                                      int new_pos) {
  (void) state;
  (void) nframes;
  (void) new_pos;
  SmfStreamer *smf_streamer = smf_streamer_container_.Fetch();
  smf_streamer->tempo_map().FillBBT(pos);
}

void JackMidiPlayer::ShutdownCallback() {
  throw std::runtime_error("Jack server shutdown");
}

void JackMidiPlayer::RequestDeactivate(std::memory_order order) noexcept {
  // Changes to pending_exception_ will be released to the main
  // thread.
  keep_running_.store(false, order);
}

void JackMidiPlayer::ConnectPort(const std::string &destination) {
  const char *own_port_name = jack_port_name(midi_port_);
  if (own_port_name == nullptr)
    throw std::runtime_error("jack_port_name failure");
  int result = jack_connect(jack_client_, own_port_name,
                            destination.c_str());
  if (result != 0 && result != EEXIST)
    throw std::runtime_error("jack_connect failure");
}

int JackMidiPlayer::StaticSyncCallback(jack_transport_state_t state,
                                       jack_position_t *pos,
                                       void *arg) noexcept {
  JackMidiPlayer *midi_player = static_cast<JackMidiPlayer *>(arg);
  return midi_player->RunAndPropagateException([=]() {
      return midi_player->SyncCallback(state, pos);
    }, false);
}

int JackMidiPlayer::StaticProcessCallback(jack_nframes_t nframes,
                                          void *arg) noexcept {
  JackMidiPlayer *midi_player = static_cast<JackMidiPlayer *>(arg);
  return midi_player->RunAndPropagateException([=]() {
      return midi_player->ProcessCallback(nframes);
    }, -1);
}

void JackMidiPlayer::StaticTimebaseCallback(jack_transport_state_t state,
                                            jack_nframes_t nframes,
                                            jack_position_t *pos,
                                            int new_pos,
                                            void *arg) noexcept {
  JackMidiPlayer *midi_player = static_cast<JackMidiPlayer *>(arg);
  midi_player->RunAndPropagateException([=]() {
      midi_player->TimebaseCallback(state, nframes, pos, new_pos);
    });
}

void JackMidiPlayer::StaticShutdownCallback(void *arg) noexcept {
  JackMidiPlayer *midi_player = static_cast<JackMidiPlayer *>(arg);
  midi_player->RunAndPropagateException([=]() {
      midi_player->ShutdownCallback();
    });
}

}
