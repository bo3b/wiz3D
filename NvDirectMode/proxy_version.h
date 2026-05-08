// NvDirectMode shared version header.
//
// Pulls the numeric components and git short SHA from bin/temp_version.h
// (regenerated each build by bin/generate_version.ps1 from VERSION.txt at
// repo root), then exposes the .rc-friendly macro names that version.rc
// expects. NvDirectMode shares the same generated header as wiz3D-proxy
// and S3DDriver so all DLLs ship with the same version stamp.
#pragma once

#include "..\bin\temp_version.h"

#define _NVDM_STR(x) #x
#define NVDM_STR(x) _NVDM_STR(x)

#define VERSION_NUMBER  WIZ3D_VERSION_MAJOR, WIZ3D_VERSION_MINOR, WIZ3D_VERSION_PATCH, WIZ3D_VERSION_QFE

#define VERSION_STRING                 \
    NVDM_STR(WIZ3D_VERSION_MAJOR) "." \
    NVDM_STR(WIZ3D_VERSION_MINOR) "." \
    NVDM_STR(WIZ3D_VERSION_PATCH) "." \
    NVDM_STR(WIZ3D_VERSION_QFE)

// Short display string for log headers, e.g. "0.1.8 (5ef3863b-dirty)"
#define DISPLAYED_VERSION              \
    NVDM_STR(WIZ3D_VERSION_MAJOR) "." \
    NVDM_STR(WIZ3D_VERSION_MINOR) "." \
    NVDM_STR(WIZ3D_VERSION_PATCH) " (" WIZ3D_GIT_SHA ")"
