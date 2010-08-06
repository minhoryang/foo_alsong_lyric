#define _WIN32_WINDOWS 0x0501
#define _WIN32_WINNT 0x0501

#pragma warning(disable:4180) //Visual C++ bug

#include <winsock2.h>
#include <windows.h>
#include <gdiplus.h>
#include <shlwapi.h>
#include <IPHlpApi.h>

#include <uxtheme.h>
#include <vssym32.h>

#include <string>
#include <vector>
#include <map>
#include <list>
#include <exception>
#include <typeinfo>
#define BOOST_BIND_ENABLE_STDCALL
#include <boost/bind.hpp>
#include <boost/signals2.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time.hpp>
#include "pugixml/pugixml.hpp"

#define foreach BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH

#include "../../sdk/foobar2000/SDK/foobar2000.h"
#include "../../sdk/foobar2000/helpers/helpers.h"
#include "../../sdk/foobar2000/columns_ui-sdk/ui_extension.h"
#include "../../3rdparty/Squirrel/sqplus/sqplus.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "comctl32.lib")

#pragma comment(lib, "../../sdk/foobar2000/shared/shared.lib")

using namespace Gdiplus;
using namespace std;
