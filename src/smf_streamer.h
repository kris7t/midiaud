#ifndef SMF_STREAMER_H_
#define SMF_STREAMER_H_

#include <string>

#include <smf.h>

#include "jack_midi_sink.h"
#include "timebase/tempo_map.h"

namespace midiaud {

class SmfStreamer {
 public:
  SmfStreamer();
  SmfStreamer(const std::string &filename);
  SmfStreamer(const SmfStreamer &) = delete;
  SmfStreamer(SmfStreamer &&) noexcept;
  ~SmfStreamer();
  SmfStreamer &operator=(const SmfStreamer &) = delete;
  SmfStreamer &operator=(SmfStreamer &&) noexcept;

  void Reposition(double seconds);
  void StopIfNeeded(bool now_playing, JackMidiSink &sink);
  void CopyToSink(double start_seconds, double end_seconds,
                  JackMidiSink &sink);

  bool initialized() const { return initialized_; }
  const timebase::TempoMap &tempo_map() const { return tempo_map_; }

 private:
  void Rewind();
  void SeekForward(double seconds);

  smf_t *smf_;
  bool initialized_;
  bool was_playing_;
  bool repositioned_;
  timebase::TempoMap tempo_map_;
};

}

#endif // SMF_STREAMER_H_
