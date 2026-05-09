/* NvDirectMode d3d11 - active-eye query bridge (mirror of d3d9/eye_state.h)
 *
 * Resolves Wiz3D_GetActiveEye from in-process nvapi[64].dll (NvApiProxy
 * spoofing the real driver). Encoding follows NV_STEREO_ACTIVE_EYE:
 *   1 = NVAPI_STEREO_EYE_RIGHT
 *   2 = NVAPI_STEREO_EYE_LEFT
 *   3 = NVAPI_STEREO_EYE_MONO  (no eye selected — render full frame)
 */

#pragma once

namespace NvDirectMode
{
    constexpr int kEyeRight = 1;
    constexpr int kEyeLeft  = 2;
    constexpr int kEyeMono  = 3;

    int GetActiveEye();

    // Stage 4 per-eye capture: register a callback with NvApiProxy's
    // Wiz3D_SetEyeChangeCallback export. Lazy resolved together with
    // Wiz3D_GetActiveEye. The handler receives (oldEye, newEye) on every
    // game call to Stereo_SetActiveEye where the value actually changes.
    typedef void (*EyeChangeHandler)(int oldEye, int newEye);
    void RegisterEyeChangeHandler(EyeChangeHandler handler);
}
