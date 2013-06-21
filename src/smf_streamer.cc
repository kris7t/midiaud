
#include "smf_streamer.h"

#include <stdexcept>

namespace midiaud {

SmfStreamer::SmfStreamer(const std::string &filename)
    : smf_(smf_load(filename.c_str())), was_playing_(false),
      repositioned_(false) {
  if (smf_ == nullptr)
    throw std::runtime_error("smf_load failed");
}

SmfStreamer::SmfStreamer(SmfStreamer &&other)
    : smf_(std::move(other.smf_)),
      was_playing_(std::move(other.was_playing_)),
      repositioned_(std::move(other.repositioned_)) {
  other.smf_ = nullptr;
}

SmfStreamer::~SmfStreamer() {
  if (smf_ != nullptr) smf_delete(smf_);
}

void SmfStreamer::Reposition(double seconds) {
  Rewind();
  SeekForward(seconds);
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
  for (;;) {
    smf_event_t *next_event = smf_peek_next_event(smf_);
    if (next_event == nullptr
        || next_event->time_seconds >= end_seconds) break;
    // Once repositioned by Reposition(), streaming is continous:
    // start_seconds corresponds to the end_seconds of the previous
    // cycle. If there is a discrepancy, send any events missed in the
    // previous cycle (in our "past") anyways.
    double offset_seconds = std::max(
        0., next_event->time_seconds - start_seconds);
    sink.WriteMidi(offset_seconds, next_event->midi_buffer,
                   next_event->midi_buffer_length);
    smf_skip_next_event(smf_);
  }
}

void SmfStreamer::Rewind() {
  smf_rewind(smf_);
}

void SmfStreamer::SeekForward(double seconds) {
  for (;;) {
    smf_event_t *next_event = smf_peek_next_event(smf_);
    if (next_event == nullptr
        || next_event->time_seconds >= seconds) break;
    smf_skip_next_event(smf_);
  }
}

}
