// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the MIT License. See License.txt in the project root for license information.

#pragma once
#define CS_ROOTWEB_CONFIG                                L"MACHINE/WEBROOT/APPHOST/"
#define CS_ROOTWEB_CONFIG_LEN                            _countof(CS_ROOTWEB_CONFIG)-1
#define CS_ASPNETCORE_SECTION                            L"system.webServer/aspNetCore"
#define CS_ASPNETCORE_PROCESS_EXE_PATH                   L"processPath"
#define CS_ASPNETCORE_PROCESS_ARGUMENTS                  L"arguments"
#define CS_ASPNETCORE_ENVIRONMENT_VARIABLE_NAME          L"name"
#define CS_ASPNETCORE_ENVIRONMENT_VARIABLE_VALUE         L"value"
#define CS_ASPNETCORE_RECYCLE_ON_FILE_CHANGE_FILE        L"file"
#define CS_ASPNETCORE_RECYCLE_ON_FILE_CHANGE_FILE_PATH   L"path"
#define CS_ASPNETCORE_HOSTING_MODEL                      L"hostingModel"

#define MAX_RAPID_FAILS_PER_MINUTE 100
#define MILLISECONDS_IN_ONE_SECOND 1000
#define MIN_PORT                   1025
#define MAX_PORT                   48000

#define TIMESPAN_IN_MILLISECONDS(x)  ((x)/((LONGLONG)(10000)))
#define TIMESPAN_IN_SECONDS(x)       ((TIMESPAN_IN_MILLISECONDS(x))/((LONGLONG)(1000)))
#define TIMESPAN_IN_MINUTES(x)       ((TIMESPAN_IN_SECONDS(x))/((LONGLONG)(60)))

enum APP_HOSTING_MODEL
{
    HOSTING_UNKNOWN = 0,
    HOSTING_IN_PROCESS,
    HOSTING_OUT_PROCESS
};

class ASPNETCORE_SHIM_CONFIG : IHttpStoredContext
{
public:
    virtual
        ~ASPNETCORE_SHIM_CONFIG();

    static
    HRESULT
        GetConfig(
            _In_  IHttpServer             *pHttpServer,
            _In_  HTTP_MODULE_ID           pModuleId,
            _In_  IHttpContext            *pHttpContext,
            _In_  HANDLE                   hEventLog,
            _Out_ ASPNETCORE_SHIM_CONFIG      **ppAspNetCoreConfig
        );

    HRESULT
        Populate(
            IHttpServer    *pHttpServer,
            IHttpContext   *pHttpContext
        );

    VOID
        CleanupStoredContext()
    {
        DereferenceConfiguration();
    }

    STRU*
        QueryApplicationPhysicalPath(
            VOID
        )
    {
        return &m_struApplicationPhysicalPath;
    }

    STRU*
        QueryApplicationPath(
            VOID
        )
    {
        return &m_struApplication;
    }

    CONST
        PCWSTR*
        QueryHostFxrArguments(
            VOID
        )
    {
        return m_ppStrArguments;
    }

    CONST
        DWORD
        QueryHostFxrArgCount(
            VOID
        )
    {
        return m_dwArgc;
    }

    VOID
        ReferenceConfiguration(
            VOID
        ) const;

    VOID
        DereferenceConfiguration(
            VOID
        ) const;

    STRU*
        QueryConfigPath()
    {
        return &m_struConfigPath;
    }

    APP_HOSTING_MODEL
        QueryHostingModel(
            VOID
        )
    {
        return m_hostingModel;
    }

    CONST
        PCWSTR
        QueryHostFxrFullPath(
            VOID
        )
    {
        return m_struHostFxrLocation.QueryStr();
    }

    VOID
        SetHostFxrArguments(
            DWORD dwArgc,
            PWSTR* ppStrArguments
        )
    {
        if (m_ppStrArguments != NULL)
        {
            delete[] m_ppStrArguments;
        }

        m_dwArgc = dwArgc;
        m_ppStrArguments = ppStrArguments;
    }

    STRU*
        QueryProcessPath(
            VOID
        )
    {
        return &m_struProcessPath;
    }

    STRU*
        QueryArguments(
            VOID
        )
    {
        return &m_struArguments;
    }

    HRESULT
        SetHostFxrFullPath(
            PCWSTR pStrHostFxrFullPath
        )
    {
        return m_struHostFxrLocation.Copy(pStrHostFxrFullPath);
    }

private:
    ASPNETCORE_SHIM_CONFIG() :
        m_cRefs(1),
        m_hostingModel(HOSTING_UNKNOWN),
        m_ppStrArguments(NULL)
    {
    }

    mutable LONG           m_cRefs;
    STRU                   m_struArguments;
    STRU                   m_struProcessPath;
    STRU                   m_struApplication;
    STRU                   m_struApplicationPhysicalPath;
    STRU                   m_struConfigPath;
    APP_HOSTING_MODEL      m_hostingModel;
    STRU                   m_struHostFxrLocation;
    PWSTR*                 m_ppStrArguments;
    DWORD                  m_dwArgc;
};

