#pragma once
class ASPNETCORE_SHIM_CONFIG
{
public:
    ASPNETCORE_SHIM_CONFIG();
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

    STRU*
        QueryApplicationPhysicalPath(
            VOID
        )
    {
        return &m_struApplicationPhysicalPath;
    }

};

