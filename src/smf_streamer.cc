
#include "smf_streamer.h"

#include <iterator>
#include <stdexcept>

#include "smf_reader-inl.h"

namespace midiaud {

SmfStreamer::SmfStreamer()
    : initialized_(false), was_playing_(false), repositioned_(false) {
}

SmfStreamer::SmfStreamer(const std::string &filename)
    : initialized_(false), was_playing_(false), repositioned_(false) {
  double ppqn;
  ReadStandardMidiFile(filename, std::back_inserter(events_), ppqn);
  next_event_ = events_.cend();

  tempo_map_ = timebase::TempoMap(ppqn);
  for (const Event &event : events_) {
    tempo_map_.AcknowledgeEvent(event);
  }
}

void SmfStreamer::Reposition(double seconds) {
  Rewind();
  SeekForwardTo(seconds);
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
  while (next_event_valid()) {
    double seconds = tempo_map_.GetTicks(next_event_->ticks()).seconds();
    if (seconds >= end_seconds) break;
    if (!next_event_->is_metadata()) {
      // Once repositioned by Reposition(), streaming is continous:
      // start_seconds corresponds to the end_seconds of the previous
      // cycle. If there is a discrepancy, send any events missed in
      // the previous cycle (in our "past") anyways.
      double offset_seconds = std::max(0., seconds - start_seconds);
      sink.WriteMidi(offset_seconds, next_event_->midi().data(),
                     next_event_->midi().size());
    }
    ++next_event_;
  }
}

void SmfStreamer::Rewind() {
  next_event_ = events_.begin();
}

void SmfStreamer::SeekForwardTo(double seconds) {
  while (next_event_valid()
         && tempo_map_.GetTicks(next_event_->ticks()).seconds() < seconds) {
    ++next_event_;
  }
}

}
