#ifndef EVENT_H_
#define EVENT_H_

#include <vector>

namespace midiaud {

typedef std::vector<uint8_t> MidiBuffer;

class Event {
 public:
  template <typename InputIterator>
  Event(double ticks, InputIterator begin, InputIterator end)
      : ticks_(ticks), midi_(begin, end) {
  }

  double ticks() const { return ticks_; }
  const MidiBuffer &midi() const { return midi_; }

  bool is_metadata() const {
    return midi_.size() > 0 && midi_[0] == 0xff;
  }

 private:
  double ticks_;
  MidiBuffer midi_;
};

} // midiaud

#endif // EVENT_H_
