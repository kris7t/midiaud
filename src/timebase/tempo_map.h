#ifndef TIMEBASE_TEMPO_MAP_H_
#define TIMEBASE_TEMPO_MAP_H_

#include <vector>

#include <jack/jack.h>

#include "event.h"
#include "timebase/position.h"

namespace midiaud {
namespace timebase {

class TempoMap {
 public:
  /**
   * Builds and empty tempo map.
   */
  TempoMap();
  /**
   * Builds tempo map based on libsmf's internal tempo map, extending
   * it to produce Jack position information fast.
   *
   * @param smf the smf object to copy tempo from.
   */
  explicit TempoMap(double ppqn);

  /**
   * Must be called with a monotone sequence of events.
   */
  void AcknowledgeEvent(const Event &event);

  Position GetSeconds(double seconds) const;
  Position GetTicks(double ticks) const;
  Position GetBBT(const BBT &bbt) const;

  void FillBBT(jack_position_t *pos) const;
  jack_nframes_t BBTToFrame(jack_position_t *pos) const;

 private:
  void AppendOrReplace(const Position &position);

  std::vector<Position> positions_;
};

} // timebase
} // midiaud

#endif // TIMEBASE_TEMPO_MAP_H_
