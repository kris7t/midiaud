#ifndef TIMEBASE_POSITION_H_
#define TIMEBASE_POSITION_H_

#include <cstdint>

namespace midiaud {
namespace timebase {

struct BBT {
  int32_t bar;
  int32_t beat;
  double tick;

  bool operator<(const BBT &rhs) const {
    return bar < rhs.bar || (
        bar == rhs.bar && (beat < rhs.beat || (
            beat == rhs.beat && tick < rhs.tick)));
  }
};

class Position {
 public:
  static constexpr int32_t kInitialBar = 1;
  static constexpr int32_t kInitialBeat = 1;
  static constexpr double kDefaultTicksPerBeat = 768;

  Position();
  explicit Position(double seconds);
  explicit Position(const BBT &bbt);

  void StartNewBar();
  void SetToSeconds(double seconds);
  void SetToTicks(double ticks);
  void IncrementBySeconds(double seconds);
  void IncrementByTicks(double ticks);
  void RoundUp();

  double SecondsToTicks(double seconds) const;
  double TicksToSeconds(double ticks) const;

  double seconds() const { return seconds_; }
  int32_t ticks() const { return ticks_; }
  const BBT &bbt() const { return bbt_; }
  double beats_per_bar() const { return beats_per_bar_; }
  void set_beats_per_bar(double value) {
    beats_per_bar_ = value;
  }
  double beat_type() const { return beat_type_; }
  void set_beat_type(double value) {
    beat_type_ = value;
  }
  double ticks_per_beat() const { return ticks_per_beat_; }
  void set_ticks_per_beat(double value) {
    ticks_per_beat_ = value;
  }
  double beats_per_minute() const { return beats_per_minute_; }
  void set_beats_per_minute(double value) {
    beats_per_minute_ = value;
  }

 private:
  double seconds_;
  int32_t ticks_;
  BBT bbt_;
  double beats_per_bar_;
  double beat_type_;
  double ticks_per_beat_;
  double beats_per_minute_;
};

struct ComparePositionBySeconds {
  bool operator()(const Position &lhs, const Position &rhs) {
    return lhs.seconds() < rhs.seconds();
  }
};

struct ComparePositionByBBT {
  bool operator()(const Position &lhs, const Position &rhs) {
    return lhs.bbt() < rhs.bbt();
  }
};

} // timebase
} // midiaud

#endif // TIMEBASE_POSITION_H_
