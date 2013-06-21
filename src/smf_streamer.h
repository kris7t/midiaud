#ifndef SMF_STREAMER_H_
#define SMF_STREAMER_H_

#include <string>

#include <smf.h>

#include "jack_midi_sink.h"

namespace midiaud {

class SmfStreamer {
 public:
  SmfStreamer(const std::string &filename);
  SmfStreamer(const SmfStreamer &) = delete;
  SmfStreamer(SmfStreamer &&);
  ~SmfStreamer();
  SmfStreamer &operator=(const SmfStreamer &) = delete;

  void Reposition(double seconds);
  void StopIfNeeded(bool now_playing, JackMidiSink &sink);
  void CopyToSink(double start_seconds, double end_seconds,
                  JackMidiSink &sink);

 private:
  void Rewind();
  void SeekForward(double seconds);

  smf_t *smf_;
  bool was_playing_;
  bool repositioned_;
};

}

#endif // SMF_STREAMER_H_
