// WizPdhHelper.h
// Clean-room implementation of Windows PDH performance counter wrapper for wiz3D.
// See if necissary, and either remove if not or implement properly if needed. This is a placeholder for now.

#ifndef WIZ_PDHHELPER_H
#define WIZ_PDHHELPER_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <pdh.h>
#include <vector>

#pragma comment(lib, "pdh.lib")

class WizPdhHelper {
private:
    PDH_HQUERY queryHandle;
    std::vector<PDH_HCOUNTER> counters;

public:
    WizPdhHelper() : queryHandle(NULL) {
        PdhOpenQuery(NULL, 0, &queryHandle);
    }

    ~WizPdhHelper() {
        if (queryHandle) PdhCloseQuery(queryHandle);
    }

    PDH_STATUS AddCounter(const wstring& counterPath) {
        if (!queryHandle) return ERROR_INVALID_HANDLE;
        PDH_HCOUNTER newCounter;
        PDH_STATUS status = PdhAddCounter(queryHandle, counterPath.c_str(), 0, &newCounter);
        if (status == ERROR_SUCCESS) {
            counters.push_back(newCounter);
        }
        return status;
    }

    PDH_STATUS Sample() {
        return queryHandle ? PdhCollectQueryData(queryHandle) : ERROR_INVALID_HANDLE;
    }
};

#endif // WIZ_PDHHELPER_H