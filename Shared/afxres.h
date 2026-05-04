// afxres.h - MFC resource compatibility shim
// MFC is not installed; this provides the standard resource defines
// that .rc files need via the Windows SDK winresrc.h
#ifndef _AFXRES_H_COMPAT
#define _AFXRES_H_COMPAT

#include <winresrc.h>

// Standard MFC resource IDs used in .rc files
#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

#endif // _AFXRES_H_COMPAT
