#ifndef SMF_STREAMER_H_
#define SMF_STREAMER_H_

#include <string>
#include <vector>

#include "event.h"
#include "jack_midi_sink.h"
#include "timebase/tempo_map.h"

namespace midiaud {

class SmfStreamer {
 public:
  SmfStreamer();
  SmfStreamer(const std::string &filename);

  void Reposition(double seconds);
  void StopIfNeeded(bool now_playing, JackMidiSink &sink);
  void CopyToSink(double start_seconds, double end_seconds,
                  JackMidiSink &sink);

  bool initialized() const { return initialized_; }
  const timebase::TempoMap &tempo_map() const { return tempo_map_; }

 private:
  void Rewind();
  void SeekForwardTo(double seconds);

  bool next_event_valid() const { return next_event_ != events_.cend(); }

  bool initialized_;
  bool was_playing_;
  bool repositioned_;
  std::vector<Event> events_;
  timebase::TempoMap tempo_map_;
  std::vector<Event>::const_iterator next_event_;
};

}

#endif // SMF_STREAMER_H_
