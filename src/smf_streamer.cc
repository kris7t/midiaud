
#include "smf_streamer.h"

#include <stdexcept>

namespace midiaud {

SmfStreamer::SmfStreamer()
    : smf_(nullptr), initialized_(false),
      was_playing_(false), repositioned_(false) {
}

SmfStreamer::SmfStreamer(const std::string &filename)
    : smf_(smf_load(filename.c_str())), initialized_(false),
      was_playing_(false), repositioned_(false) {
  if (smf_ == nullptr)
    throw std::runtime_error("smf_load failed");
  tempo_map_ = timebase::TempoMap(smf_);
}

SmfStreamer::SmfStreamer(SmfStreamer &&other) noexcept
    : smf_(std::move(other.smf_)),
      initialized_(std::move(other.initialized_)),
      was_playing_(std::move(other.was_playing_)),
      repositioned_(std::move(other.repositioned_)),
      tempo_map_(std::move(other.tempo_map_)) {
  other.smf_ = nullptr;
}

SmfStreamer &SmfStreamer::operator=(SmfStreamer &&other) noexcept {
  if (this != &other) {
    if (smf_ != nullptr) smf_delete(smf_);
    smf_ = std::move(other.smf_);
    initialized_ = std::move(other.initialized_);
    was_playing_ = std::move(other.was_playing_);
    repositioned_ = std::move(other.repositioned_);
    tempo_map_ = std::move(other.tempo_map_);
    other.smf_ = nullptr;
  }
  return *this;
}

SmfStreamer::~SmfStreamer() {
  if (smf_ != nullptr) smf_delete(smf_);
}

void SmfStreamer::Reposition(double seconds) {
  Rewind();
  SeekForward(seconds);
  initialized_ = true;
  repositioned_ = true;
}

void SmfStreamer::StopIfNeeded(bool now_playing, JackMidiSink &sink) {
  if (repositioned_ || (was_playing_ && !now_playing))
    sink.WriteGlobalSoundOff(0);
  repositioned_ = false;
  was_playing_ = now_playing;
}

void SmfStreamer::CopyToSink(double start_seconds, double end_seconds,
                             JackMidiSink &sink) {
  if (smf_ == nullptr) return;
  for (;;) {
    smf_event_t *next_event = smf_peek_next_event(smf_);
    if (next_event == nullptr
        || next_event->time_seconds >= end_seconds) break;
    if (!smf_event_is_metadata(next_event)) {
      // Once repositioned by Reposition(), streaming is continous:
      // start_seconds corresponds to the end_seconds of the previous
      // cycle. If there is a discrepancy, send any events missed in
      // the previous cycle (in our "past") anyways.
      double offset_seconds = std::max(
          0., next_event->time_seconds - start_seconds);
      sink.WriteMidi(offset_seconds, next_event->midi_buffer,
                     next_event->midi_buffer_length);
    }
    smf_skip_next_event(smf_);
  }
}

void SmfStreamer::Rewind() {
  if (smf_ == nullptr) return;
  smf_rewind(smf_);
}

void SmfStreamer::SeekForward(double seconds) {
  if (smf_ == nullptr) return;
  for (;;) {
    smf_event_t *next_event = smf_peek_next_event(smf_);
    if (next_event == nullptr
        || next_event->time_seconds >= seconds) break;
    smf_skip_next_event(smf_);
  }
}

}
