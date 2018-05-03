#pragma once

enum APP_HOSTING_MODEL
{
    HOSTING_UNKNOWN = 0,
    HOSTING_IN_PROCESS,
    HOSTING_OUT_PROCESS
};

class ASPNETCORE_SHIM_CONFIG : IHttpStoredContext
{
public:
    ASPNETCORE_SHIM_CONFIG();
    virtual
        ~ASPNETCORE_SHIM_CONFIG();

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
    mutable LONG           m_cRefs;
    DWORD                  m_dwRequestTimeoutInMS;
    DWORD                  m_dwStartupTimeLimitInMS;
    DWORD                  m_dwShutdownTimeLimitInMS;
    DWORD                  m_dwRapidFailsPerMinute;
    DWORD                  m_dwProcessesPerApplication;
    STRU                   m_struArguments;
    STRU                   m_struProcessPath;
    STRU                   m_struStdoutLogFile;
    STRU                   m_struApplication;
    STRU                   m_struApplicationPhysicalPath;
    STRU                   m_struApplicationVirtualPath;
    STRU                   m_struConfigPath;
    BOOL                   m_fStdoutLogEnabled;
    BOOL                   m_fForwardWindowsAuthToken;
    BOOL                   m_fDisableStartUpErrorPage;
    BOOL                   m_fWindowsAuthEnabled;
    BOOL                   m_fBasicAuthEnabled;
    BOOL                   m_fAnonymousAuthEnabled;
    BOOL                   m_fWebSocketEnabled;
    APP_HOSTING_MODEL      m_hostingModel;
    ENVIRONMENT_VAR_HASH*  m_pEnvironmentVariables;
    STRU                   m_struHostFxrLocation;
    PWSTR*                 m_ppStrArguments;
    DWORD                  m_dwArgc;
};

