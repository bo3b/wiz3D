#pragma once
// Stub: transformation.h only needs cv::Mat and cv::Matx types from opencv2/core.hpp.
// The full opencv2/opencv.hpp pulls in videostab which has headers broken under MSVC.
// Suppress C4946 (reinterpret_cast between related classes in cv::Scalar) which
// Common.props promotes to an error — it is harmless in this read-only OpenCV context.
#pragma warning(push)
#pragma warning(disable: 4946)
#include "opencv2/core.hpp"
#pragma warning(pop)
