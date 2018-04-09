// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the MIT License. See License.txt in the project root for license information.

#include "stdafx.h"
#include "WindowsShellApi.h"

LPWSTR* WindowsShellApi::WindowsCommandLineToArgvW(LPCWSTR lpCmdLine, int * pNumArgs) noexcept
{
    return CommandLineToArgvW(lpCmdLine, pNumArgs);
}

BOOL WindowsShellApi::WindowsPathIsRelative(LPCWSTR pwzPath) noexcept
{
    return PathIsRelative(pwzPath);
}
