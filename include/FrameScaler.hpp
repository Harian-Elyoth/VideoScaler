#ifndef FRAMESCALER_HPP
#define FRAMESCALER_HPP

#include "ScalerUtils.hpp"

class FrameScaler {
public:
    static Frame resize(const Frame& inputFrame, int newWidth, int newHeight);
};

#endif // FRAMESCALER_HPP