/* NvDirectMode — shared AnaglyphColour x AnaglyphMethod coefficient table
 *
 * Six float3 rows per [colour][method] cell:
 *   lR / lG / lB = how much of left-eye RGB contributes to output R / G / B
 *   rR / rG / rB = how much of right-eye RGB contributes to output R / G / B
 *
 * Same coefficient values used in AmdQbProxy/AmdQbProxy.cpp — Dubois 2001
 * spectral fits for the 7 methods, with matching adaptations for the three
 * glasses colour pairs (Red/Cyan, Green/Magenta, Amber/Blue).
 *
 * Header-only so each per-API proxy (d3d9 / d3d10 / d3d11 / opengl32) can
 * pull in the same values without copying. NvDirectMode_AnaglyphMatrix is
 * agnostic of D3D / GL types — translates to whatever per-eye constant
 * buffer / uniform block format the API uses.
 */

#pragma once

namespace NvDirectMode
{
    struct AnaglyphMatrix { float lR[3], lG[3], lB[3], rR[3], rG[3], rB[3]; };

    // Indexed [AnaglyphColour 0..2][AnaglyphMethod 0..6].
    // Colour: 0=Red/Cyan, 1=Green/Magenta, 2=Amber/Blue
    // Method: 0=Dubois, 1=Compromise, 2=Color, 3=HalfColor, 4=Optimised, 5=Grey, 6=True
    //
    // 'static' (not 'inline') so this header works with C++14 — gives each
    // including TU its own copy of the table (~1.5 KB, trivial). Switch to
    // 'inline const' if NvDirectMode's project files ever bump to /std:c++17.
    static const AnaglyphMatrix kAnaglyphMatrices[3][7] =
    {
        // ---- AC_RED_CYAN [0] ----
        {
            // AM_DUBOIS — spectral least-squares fit (Dubois 2001); best ghosting suppression
            {{ 0.456f, 0.500f, 0.176f}, {-0.040f,-0.038f,-0.016f}, {-0.015f,-0.021f,-0.016f},
             {-0.043f,-0.088f,-0.002f}, { 0.378f, 0.734f, 0.018f}, {-0.072f,-0.113f, 1.226f}},
            // AM_COMPROMISE — Ahtik 2011; balanced glasses/no-glasses
            {{ 0.439f, 0.447f, 0.148f}, { 0.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 0.000f},
             { 0.000f, 0.000f, 0.000f}, { 0.095f, 0.934f,-0.005f}, {-0.018f,-0.028f, 1.057f}},
            // AM_COLOR — direct channel passthrough; severe colour fringing
            {{ 1.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 0.000f},
             { 0.000f, 0.000f, 0.000f}, { 0.000f, 1.000f, 0.000f}, { 0.000f, 0.000f, 1.000f}},
            // AM_HALF_COLOR — left desaturated, right keeps colour
            {{ 0.299f, 0.587f, 0.114f}, { 0.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 0.000f},
             { 0.000f, 0.000f, 0.000f}, { 0.000f, 1.000f, 0.000f}, { 0.000f, 0.000f, 1.000f}},
            // AM_OPTIMISED — Wimmer weighted-channel
            {{ 0.000f, 0.700f, 0.300f}, { 0.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 0.000f},
             { 0.000f, 0.000f, 0.000f}, { 0.000f, 1.000f, 0.000f}, { 0.000f, 0.000f, 1.000f}},
            // AM_GREY — both eyes greyscale; near-zero ghosting, no colour
            {{ 0.299f, 0.587f, 0.114f}, { 0.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 0.000f},
             { 0.000f, 0.000f, 0.000f}, { 0.299f, 0.587f, 0.114f}, { 0.299f, 0.587f, 0.114f}},
            // AM_TRUE — classic single-channel luma; max ghosting, historical
            {{ 0.299f, 0.587f, 0.114f}, { 0.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 0.000f},
             { 0.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 0.000f}, { 0.299f, 0.587f, 0.114f}},
        },
        // ---- AC_GREEN_MAGENTA [1] ----
        {
            // AM_DUBOIS — approximated from Dubois G/M filter measurements
            {{-0.063f,-0.162f, 0.042f}, { 0.285f, 0.665f, 0.150f}, {-0.015f,-0.027f, 0.021f},
             { 0.529f, 0.705f,-0.047f}, {-0.016f,-0.015f,-0.065f}, { 0.009f, 0.076f, 0.937f}},
            // AM_COMPROMISE
            {{ 0.000f, 0.000f, 0.000f}, { 0.146f, 0.738f, 0.141f}, { 0.000f, 0.000f, 0.000f},
             { 0.882f, 0.176f,-0.012f}, { 0.000f, 0.000f, 0.000f}, { 0.002f, 0.019f, 0.984f}},
            // AM_COLOR
            {{ 0.000f, 0.000f, 0.000f}, { 0.000f, 1.000f, 0.000f}, { 0.000f, 0.000f, 0.000f},
             { 1.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 1.000f}},
            // AM_HALF_COLOR
            {{ 0.000f, 0.000f, 0.000f}, { 0.299f, 0.587f, 0.114f}, { 0.000f, 0.000f, 0.000f},
             { 1.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 1.000f}},
            // AM_OPTIMISED
            {{ 0.000f, 0.000f, 0.000f}, { 0.000f, 0.700f, 0.300f}, { 0.000f, 0.000f, 0.000f},
             { 1.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 1.000f}},
            // AM_GREY
            {{ 0.000f, 0.000f, 0.000f}, { 0.299f, 0.587f, 0.114f}, { 0.000f, 0.000f, 0.000f},
             { 0.299f, 0.587f, 0.114f}, { 0.000f, 0.000f, 0.000f}, { 0.299f, 0.587f, 0.114f}},
            // AM_TRUE
            {{ 0.000f, 0.000f, 0.000f}, { 0.299f, 0.587f, 0.114f}, { 0.000f, 0.000f, 0.000f},
             { 0.299f, 0.587f, 0.114f}, { 0.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 0.000f}},
        },
        // ---- AC_AMBER_BLUE [2] ----
        {
            // AM_DUBOIS — approximated from Dubois A/B filter measurements
            {{ 1.062f, 0.366f,-0.057f}, {-0.063f,-0.019f, 0.019f}, {-0.003f,-0.016f, 0.031f},
             {-0.390f,-0.350f, 0.055f}, { 0.468f, 0.246f, 0.000f}, {-0.018f, 0.102f, 0.902f}},
            // AM_COMPROMISE
            {{ 0.840f, 0.238f, 0.014f}, { 0.059f, 0.642f, 0.033f}, { 0.000f, 0.000f, 0.000f},
             { 0.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 0.000f}, {-0.005f, 0.026f, 0.976f}},
            // AM_COLOR
            {{ 1.000f, 0.000f, 0.000f}, { 0.000f, 1.000f, 0.000f}, { 0.000f, 0.000f, 0.000f},
             { 0.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 1.000f}},
            // AM_HALF_COLOR
            {{ 0.299f, 0.587f, 0.114f}, { 0.299f, 0.587f, 0.114f}, { 0.000f, 0.000f, 0.000f},
             { 0.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 1.000f}},
            // AM_OPTIMISED
            {{ 1.000f, 0.000f, 0.000f}, { 0.000f, 1.000f, 0.000f}, { 0.000f, 0.000f, 0.000f},
             { 0.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 1.000f}},
            // AM_GREY
            {{ 0.299f, 0.587f, 0.114f}, { 0.299f, 0.587f, 0.114f}, { 0.000f, 0.000f, 0.000f},
             { 0.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 0.000f}, { 0.299f, 0.587f, 0.114f}},
            // AM_TRUE
            {{ 0.299f, 0.587f, 0.114f}, { 0.299f, 0.587f, 0.114f}, { 0.000f, 0.000f, 0.000f},
             { 0.000f, 0.000f, 0.000f}, { 0.000f, 0.000f, 0.000f}, { 0.299f, 0.587f, 0.114f}},
        },
    };
}
