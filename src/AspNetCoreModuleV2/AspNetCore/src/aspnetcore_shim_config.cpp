// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the MIT License. See License.txt in the project root for license information.

#include "precomp.hxx"

ASPNETCORE_SHIM_CONFIG::~ASPNETCORE_SHIM_CONFIG()
{
    if (m_ppStrArguments != NULL)
    {
        delete[] m_ppStrArguments;
        m_ppStrArguments = NULL;
    }
}

VOID
ASPNETCORE_SHIM_CONFIG::ReferenceConfiguration(
    VOID
) const
{
    InterlockedIncrement(&m_cRefs);
}

VOID
ASPNETCORE_SHIM_CONFIG::DereferenceConfiguration(
    VOID
) const
{
    DBG_ASSERT(m_cRefs != 0);
    LONG cRefs = 0;
    if ((cRefs = InterlockedDecrement(&m_cRefs)) == 0)
    {
        delete this;
    }
}

HRESULT
ASPNETCORE_SHIM_CONFIG::GetConfig(
    _In_  IHttpServer             *pHttpServer,
    _In_  HTTP_MODULE_ID           pModuleId,
    _In_  IHttpApplication        *pHttpApplication,
    _In_  HANDLE                   hEventLog,
    _Out_ ASPNETCORE_SHIM_CONFIG **ppAspNetCoreShimConfig
)
{
    HRESULT                 hr = S_OK;
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

    hr = pAspNetCoreShimConfig->Populate(pHttpServer, pHttpApplication);
    if (FAILED(hr))
    {
        goto Finished;
    }

    // Modify config for inprocess.'
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
            "ASPNETCORE_SHIM_CONFIG::GetConfig, set config to ModuleContext");

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
    IHttpApplication * pHttpApplication
)
{
    STACK_STRU(strHostingModel, 300);
    HRESULT                         hr = S_OK;
    STRU                            strApplicationFullPath;
    IAppHostAdminManager           *pAdminManager = NULL;
    IAppHostElement                *pAspNetCoreElement = NULL;
    DWORD                           dwCounter = 0;
    DWORD                           dwPosition = 0;
    WCHAR*                          pszPath = NULL;
    BSTR                            bstrAspNetCoreSection = NULL;

    pAdminManager = pHttpServer->GetAdminManager();
    hr = m_struConfigPath.Copy(pHttpApplication->GetAppConfigPath());
    if (FAILED(hr))
    {
        goto Finished;
    }

    hr = m_struApplicationPhysicalPath.Copy(pHttpApplication->GetApplicationPhysicalPath());
    if (FAILED(hr))
    {
        goto Finished;
    }

    pszPath = m_struConfigPath.QueryStr();
    while (pszPath[dwPosition] != NULL)
    {
        if (pszPath[dwPosition] == '/')
        {
            dwCounter++;
            if (dwCounter == 4)
                break;
        }
        dwPosition++;
    }

    bstrAspNetCoreSection = SysAllocString(CS_ASPNETCORE_SECTION);

    hr = pAdminManager->GetAdminSection(bstrAspNetCoreSection,
        m_struConfigPath.QueryStr(),
        &pAspNetCoreElement);
    if (FAILED(hr))
    {
        goto Finished;
    }

    hr = GetElementStringProperty(pAspNetCoreElement,
        CS_ASPNETCORE_PROCESS_EXE_PATH,
        &m_struProcessPath);
    if (FAILED(hr))
    {
        goto Finished;
    }

    hr = GetElementStringProperty(pAspNetCoreElement,
        CS_ASPNETCORE_HOSTING_MODEL,
        &strHostingModel);
    if (FAILED(hr))
    {
        // Swallow this error for backward compatability
        // Use default behavior for empty string
        hr = S_OK;
    }

    if (strHostingModel.IsEmpty() || strHostingModel.Equals(L"outofprocess", TRUE))
    {
        m_hostingModel = HOSTING_OUT_PROCESS;
    }
    else if (strHostingModel.Equals(L"inprocess", TRUE))
    {
        m_hostingModel = HOSTING_IN_PROCESS;
    }
    else
    {
        // block unknown hosting value
        hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        goto Finished;
    }

    hr = GetElementStringProperty(pAspNetCoreElement,
        CS_ASPNETCORE_PROCESS_ARGUMENTS,
        &m_struArguments);
    if (FAILED(hr))
    {
        goto Finished;
    }

Finished:

    if (pAspNetCoreElement != NULL)
    {
        pAspNetCoreElement->Release();
        pAspNetCoreElement = NULL;
    }

    return hr;
}

