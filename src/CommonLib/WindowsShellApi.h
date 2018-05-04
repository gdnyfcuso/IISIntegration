// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the MIT License. See License.txt in the project root for license information.

#pragma once
class WindowsShellApi : public WindowsShellApiInterface
{
public:
    virtual LPWSTR* WindowsCommandLineToArgvW(_In_ LPCWSTR lpCmdLine, _Out_ int* pNumArgs) noexcept;
    virtual BOOL WindowsPathIsRelative(_In_ LPCWSTR pwzPath) noexcept;
};

