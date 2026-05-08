/* NvDirectMode opengl32 - active-eye query bridge (mirror of d3d9/d3d10/d3d11) */

#pragma once

namespace NvDirectMode
{
    constexpr int kEyeRight = 1;
    constexpr int kEyeLeft  = 2;
    constexpr int kEyeMono  = 3;

    int GetActiveEye();
}
