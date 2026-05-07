// wiz3D version-info shim.
//
// The numeric version components and git SHA come from bin/temp_version.h,
// which is regenerated each build by bin/generate_version.ps1 (called from
// the Preparing project's pre-build step). The single source of truth is the
// repo-root VERSION file.
//
// Don't edit PRODUCT_VERSION_MAJOR/MINOR/BUILD here -- they're aliases set
// by temp_version.h. Bump the version in the VERSION file instead.

#include "..\bin\temp_version.h"

// Pre-release suffix shown next to the version string (e.g. "-rc1", "-beta").
// Leave empty for stable releases.
#define VERSION_NAME_SUFFIX     ""
