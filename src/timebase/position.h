#ifndef TIMEBASE_POSITION_H_
#define TIMEBASE_POSITION_H_

namespace midiaud {
namespace timebase {

struct BBT {
  int32_t bar;
  int32_t beat;
  int32_t tick;

  bool operator<(const BBT &rhs) const {
    return bar < rhs.bar || (
        bar == rhs.bar && (beat < rhs.beat || (
            beat == rhs.beat && tick < rhs.tick)));
  }
};

struct Position {
  double seconds;
  int32_t ticks;
  BBT bbt;
  double beats_per_bar;
  double beat_type;
  double ticks_per_beat;
  double beats_per_minute;
};

struct ComparePositionBySeconds {
  bool operator()(const Position &lhs, const Position &rhs) {
    return lhs.seconds < rhs.seconds;
  }
};

struct ComparePositionByBBT {
  bool operator()(const Position &lhs, const Position &rhs) {
    return lhs.bbt < rhs.bbt;
  }
};

} // timebase
} // midiaud

#endif // TIMEBASE_POSITION_H_
