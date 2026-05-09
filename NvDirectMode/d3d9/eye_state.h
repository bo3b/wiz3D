/* NvDirectMode - active-eye query bridge
 *
 * Resolves Wiz3D_GetActiveEye from the in-process nvapi[64].dll (which is
 * actually our NvApiProxy spoofing the real driver). Returns the current
 * value of NvAPI_Stereo_SetActiveEye that the game last set, encoded with
 * NVIDIA's NV_STEREO_ACTIVE_EYE values:
 *   1 = NVAPI_STEREO_EYE_RIGHT
 *   2 = NVAPI_STEREO_EYE_LEFT
 *   3 = NVAPI_STEREO_EYE_MONO  (no eye selected — render full frame)
 *
 * If NvApiProxy isn't loaded (or the export isn't found — older proxy build,
 * or the real NVIDIA driver got there first) we fall back to MONO so the
 * proxy degrades gracefully to "no per-eye routing".
 */

#pragma once

namespace NvDirectMode
{
    constexpr int kEyeRight = 1;
    constexpr int kEyeLeft  = 2;
    constexpr int kEyeMono  = 3;

    // Cheap on every call — caches the resolved fn pointer once nvapi is
    // present, but re-checks GetModuleHandle each call so a late-loading
    // nvapi (some games load it lazily) still gets picked up.
    int GetActiveEye();

    // Stage 4 per-eye capture: NvApiProxy callback registration. Mirrors
    // d3d11/d3d10 — the SwapChainProxy/Device9Proxy registers a handler
    // that fires every time the game changes Stereo_SetActiveEye.
    typedef void (*EyeChangeHandler)(int oldEye, int newEye);
    void RegisterEyeChangeHandler(EyeChangeHandler handler);
}
