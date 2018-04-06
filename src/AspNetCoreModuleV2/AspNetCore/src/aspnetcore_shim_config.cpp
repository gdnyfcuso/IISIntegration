#include "precomp.hxx"

HRESULT
ASPNETCORE_SHIM_CONFIG::GetConfig(
    _In_  IHttpServer             *pHttpServer,
    _In_  HTTP_MODULE_ID           pModuleId,
    _In_  IHttpContext            *pHttpContext,
    _In_  HANDLE                   hEventLog,
    _Out_ ASPNETCORE_SHIM_CONFIG **ppAspNetCoreShimConfig
)
{
    HRESULT                 hr = S_OK;
    IHttpApplication       *pHttpApplication = pHttpContext->GetApplication();
    ASPNETCORE_SHIM_CONFIG *pAspNetCoreShimConfig = NULL;
    STRU                    struHostFxrDllLocation;
    PWSTR*                  pwzArgv;
    DWORD                   dwArgCount;

    if (ppAspNetCoreShimConfig == NULL)
    {
        hr = E_INVALIDARG;
        goto Finished;
    }

    *ppAspNetCoreShimConfig = NULL;

    // potential bug if user sepcific config at virtual dir level
    pAspNetCoreShimConfig = (ASPNETCORE_SHIM_CONFIG*)
        pHttpApplication->GetModuleContextContainer()->GetModuleContext(pModuleId);

    if (pAspNetCoreShimConfig != NULL)
    {
        *ppAspNetCoreShimConfig = pAspNetCoreShimConfig;
        pAspNetCoreShimConfig = NULL;
        goto Finished;
    }

    pAspNetCoreShimConfig = new ASPNETCORE_SHIM_CONFIG;
    if (pAspNetCoreShimConfig == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Finished;
    }

    hr = pAspNetCoreShimConfig->Populate(pHttpServer, pHttpContext);
    if (FAILED(hr))
    {
        goto Finished;
    }

    // Modify config for inprocess.
    // TODO remove this 
    if (pAspNetCoreShimConfig->QueryHostingModel() == APP_HOSTING_MODEL::HOSTING_IN_PROCESS)
    {
        if (FAILED(hr = HOSTFXR_UTILITY::GetHostFxrParameters(
            hEventLog,
            pAspNetCoreShimConfig->QueryProcessPath()->QueryStr(),
            pAspNetCoreShimConfig->QueryApplicationPhysicalPath()->QueryStr(),
            pAspNetCoreShimConfig->QueryArguments()->QueryStr(),
            &struHostFxrDllLocation,
            &dwArgCount,
            &pwzArgv)))
        {
            goto Finished;
        }

        if (FAILED(hr = pAspNetCoreShimConfig->SetHostFxrFullPath(struHostFxrDllLocation.QueryStr())))
        {
            goto Finished;
        }

        pAspNetCoreShimConfig->SetHostFxrArguments(dwArgCount, pwzArgv);
    }

    hr = pHttpApplication->GetModuleContextContainer()->
        SetModuleContext(pAspNetCoreShimConfig, pModuleId);
    if (FAILED(hr))
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_ALREADY_ASSIGNED))
        {
            delete pAspNetCoreShimConfig;

            pAspNetCoreShimConfig = (ASPNETCORE_SHIM_CONFIG*)pHttpApplication->
                GetModuleContextContainer()->
                GetModuleContext(pModuleId);

            _ASSERT(pAspNetCoreShimConfig != NULL);

            hr = S_OK;
        }
        else
        {
            goto Finished;
        }
    }
    else
    {
        DebugPrintf(ASPNETCORE_DEBUG_FLAG_INFO,
            "ASPNETCORE_CONFIG::GetConfig, set config to ModuleContext");
        // set appliction info here instead of inside Populate()
        // as the destructor will delete the backend process
        hr = pAspNetCoreShimConfig->QueryApplicationPath()->Copy(pHttpApplication->GetApplicationId());
        if (FAILED(hr))
        {
            goto Finished;
        }
    }

    *ppAspNetCoreShimConfig = pAspNetCoreShimConfig;
    pAspNetCoreShimConfig = NULL;

Finished:

    if (pAspNetCoreShimConfig != NULL)
    {
        delete pAspNetCoreShimConfig;
        pAspNetCoreShimConfig = NULL;
    }

    return hr;
}


HRESULT
ASPNETCORE_SHIM_CONFIG::Populate(
    IHttpServer    *pHttpServer,
    IHttpContext   *pHttpContext
)
{
    HRESULT hr = S_OK;
    return hr;
}
