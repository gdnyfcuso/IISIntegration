// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the MIT License. See License.txt in the project root for license information.

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <Windows.h>
#include <httpserv.h>
#include <wchar.h>
#include <vector>
#include <shellapi.h>
#include <sstream>
#include "Shlwapi.h"
#include "hashtable.h"
#include "stringu.h"
#include "stringa.h"
#include "multisz.h"
#include "dbgutil.h"
#include "ahutil.h"
#include "hashfn.h"
#include "irequesthandler.h"
#include "requesthandler.h"
#include "iapplication.h"
#include "application.h"
#include "SRWLockWrapper.h"
#include "environmentvariablehash.h"
#include "utility.h"
#include "fx_ver.h"
#include "hostfxr_utility.h"
#include "resources.h"
#include "aspnetcore_msg.h"
