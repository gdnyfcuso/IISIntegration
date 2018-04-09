// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the MIT License. See License.txt in the project root for license information.

#include "precomp.hxx"

__override
HRESULT
ASPNET_CORE_PROXY_MODULE_FACTORY::GetHttpModule(
    CHttpModule **      ppModule,
    IModuleAllocator *  pAllocator
)
{
    ASPNET_CORE_PROXY_MODULE *pModule = new (pAllocator) ASPNET_CORE_PROXY_MODULE(m_pFileApi, m_pHostfxrUtility);
    if (pModule == NULL)
    {
        return E_OUTOFMEMORY;
    }

    *ppModule = pModule;
    return S_OK;
}

__override
VOID
ASPNET_CORE_PROXY_MODULE_FACTORY::Terminate(
    VOID
)
/*++

Routine description:

Function called by IIS for global (non-request-specific) notifications

Arguments:

None.

Return value:

None

--*/
{
    /* FORWARDING_HANDLER::StaticTerminate();

    WEBSOCKET_HANDLER::StaticTerminate();*/

    ALLOC_CACHE_HANDLER::StaticTerminate();

    delete this;
}

ASPNET_CORE_PROXY_MODULE_FACTORY::~ASPNET_CORE_PROXY_MODULE_FACTORY()
{
    if (m_pHostfxrUtility != NULL)
    {
        delete m_pHostfxrUtility;
    }

    if (m_pFileApi != NULL)
    {
        delete m_pFileApi;
    }
}
