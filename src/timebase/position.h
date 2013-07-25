#ifndef TIMEBASE_POSITION_H_
#define TIMEBASE_POSITION_H_

#include <cstdint>

namespace midiaud {
namespace timebase {

struct BBT {
  static constexpr int32_t kInitialBar = 1;
  static constexpr int32_t kInitialBeat = 1;

  int32_t bar;
  int32_t beat;
  double tick;

  bool operator<(const BBT &rhs) const {
    return bar < rhs.bar || (
        bar == rhs.bar && (beat < rhs.beat || (
            beat == rhs.beat && tick < rhs.tick)));
  }

  BBT last_bar_start() const {
    return {bar, kInitialBeat, 0};
  }
};

class Position {
 public:
  struct ConstructFromSeconds {
  };

  struct ConstructFromTicks {
  };

  static constexpr double kDefaultTicksPerBeat = 768;

  Position();
  Position(ConstructFromSeconds, double seconds);
  Position(ConstructFromTicks, double ticks);
  explicit Position(const BBT &bbt);

  void StartNewBar();
  void SetToSeconds(double seconds);
  void SetToTicks(double ticks);
  void SetToBBT(const BBT &bbt);
  /**
   * It is a programming error to call this method with negative
   * `seconds`.
   */
  void IncrementBySeconds(double seconds);
  /**
   * It is a programming error to call this method with negative
   * `seconds`.
   */
  void IncrementByTicks(double ticks);
  /**
   * Rounds up to the nearest whole tick in the bar.
   */
  void RoundUp();

  double SecondsToTicks(double seconds) const;
  double TicksToSeconds(double ticks) const;

  void PpqnChange(double ppqn);
  void TimeSignatureChange(double beats_per_bar, double beat_type,
                           double clocks_per_metronome_tick,
                           double thirty_seconds_per_midi_quarter);
  void TempoChange(double microseconds_per_midi_quarter);

  double seconds() const { return seconds_; }
  double ticks() const { return ticks_; }
  const BBT &bbt() const { return bbt_; }
  double beats_per_bar() const { return beats_per_bar_; }
  double beat_type() const { return beat_type_; }
  double ticks_per_beat() const { return ticks_per_beat_; }
  double beats_per_minute() const { return beats_per_minute_; }
  double microseconds_per_midi_quarter() const {
    return microseconds_per_midi_quarter_;
  }
  double beats_per_midi_quarter() const {
    return beats_per_midi_quarter_;
  }
  double ppqn() const { return ppqn_; }

 private:
  double seconds_;
  double ticks_;
  BBT bbt_;
  double beats_per_bar_;
  double beat_type_;
  double ticks_per_beat_;
  double beats_per_minute_;
  double microseconds_per_midi_quarter_;
  double beats_per_midi_quarter_;
  double ppqn_;
};

/**
 * Functor class for `std::binary_search`.
 */
template <typename FieldType, FieldType (Position::*Member)() const>
struct ComparePosition {
  bool operator()(const Position &lhs, const Position &rhs) {
    return (lhs.*Member)() < (rhs.*Member)();
  }
};

typedef ComparePosition<double, &Position::seconds> ComparePositionBySeconds;
typedef ComparePosition<double, &Position::ticks> ComparePositionByTicks;
typedef ComparePosition<const BBT &, &Position::bbt> ComparePositionByBBT;

} // timebase
} // midiaud

#endif // TIMEBASE_POSITION_H_
