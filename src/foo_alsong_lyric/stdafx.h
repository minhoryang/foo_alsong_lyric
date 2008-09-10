#define _WIN32_WINDOWS 0x0500
#define _WIN32_WINNT 0x0500

#include <winsock2.h>
#include <windows.h>
#include <gdiplus.h>
#include <shlwapi.h>
#include <IPHlpApi.h>

#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include "../../foobar2000/foobar2000/SDK/foobar2000.h"
#include "../../foobar2000/foobar2000/helpers/helpers.h"
#include "../../foobar2000/foobar2000/columns_ui_sdk/ui_extension.h"

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comdlg32.lib")

#ifndef _DEBUG
#pragma comment(lib, "../../foobar2000/foobar2000/shared/shared.lib")
#pragma comment(lib, "../release/foobar2000_component_client/foobar2000_component_client.lib")
#pragma comment(lib, "../release/foobar2000_SDK/foobar2000_SDK.lib")
#pragma comment(lib, "../release/foobar2000_sdk_helpers/foobar2000_sdk_helpers.lib")
#pragma comment(lib, "../release/pfc/pfc.lib")
#pragma comment(lib, "../release/columns_ui_sdk/columns_ui_sdk.lib")
#else
#pragma comment(lib, "../../foobar2000/foobar2000/shared/shared.lib")
#pragma comment(lib, "../debug/foobar2000_component_client/foobar2000_component_client.lib")
#pragma comment(lib, "../debug/foobar2000_SDK/foobar2000_SDK.lib")
#pragma comment(lib, "../debug/foobar2000_sdk_helpers/foobar2000_sdk_helpers.lib")
#pragma comment(lib, "../debug/pfc/pfc.lib")
#pragma comment(lib, "../debug/columns_ui_sdk/columns_ui_sdk.lib")
#endif