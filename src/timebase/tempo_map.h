#ifndef TIMEBASE_TEMPO_MAP_H_
#define TIMEBASE_TEMPO_MAP_H_

#include <vector>

#include <jack/jack.h>

#include <smf.h>

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
  explicit TempoMap(smf_t *smf);

  void FillBBT(jack_position_t *pos) const;
  jack_nframes_t BBTToFrame(jack_position_t *pos) const;

 private:
  Position Get(double seconds) const;

  std::vector<Position> positions_;
};

} // timebase
} // midiaud

#endif // TIMEBASE_TEMPO_MAP_H_
