#pragma once

#include <algorithm>
template <typename sample_t>
inline const sample_t linearXFade4(sample_t one, sample_t two, sample_t three,
                                   sample_t four, sample_t mixAmount) {
  mixAmount = mixAmount * 4.0;
  sample_t twoMix = fmin(fmax(mixAmount - 1.0, 0.0), 1.0);
  sample_t threeMix = fmin(fmax(mixAmount - 2.0, 0.0), 1.0);
  sample_t fourMix = fmin(fmax(mixAmount - 3.0, 0.0), 1.0);
  sample_t channelOneMixLevel = fmax(1.0 - mixAmount, 0.0);
  sample_t channelTwoMixLevel = twoMix - threeMix;
  sample_t channelThreeMixLevel = threeMix - fourMix;
  sample_t channelFourMixLevel = fourMix;
  return (one * channelOneMixLevel) + (two * channelTwoMixLevel) +
         (three * channelThreeMixLevel) + (four * channelFourMixLevel);
};
