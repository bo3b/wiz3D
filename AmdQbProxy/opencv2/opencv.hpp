#pragma once
// Stub: SR SDK's transformation.h pulls in opencv2/opencv.hpp which then drags
// in videostab headers that don't compile under MSVC. We only need cv::Mat and
// cv::Matx types from opencv2/core.hpp.
// Suppress C4946 (reinterpret_cast between related classes) which Common.props
// promotes to an error — harmless in this read-only OpenCV context.
#pragma warning(push)
#pragma warning(disable: 4946)
#include "opencv2/core.hpp"
#pragma warning(pop)
