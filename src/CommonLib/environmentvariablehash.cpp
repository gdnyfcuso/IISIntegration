// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the MIT License. See License.txt in the project root for license information.

#include "stdafx.h"
static
HRESULT
ENVIRONMENT_VAR_HASH::InitEnvironmentVariablesTable
(
    _In_ ENVIRONMENT_VAR_HASH*      pInEnvironmentVarTable,
    BOOL                            fWindowsAuthEnabled,
    BOOL                            fBasicAuthEnabled,
    BOOL                            fAnonymousAuthEnabled,
    ENVIRONMENT_VAR_HASH**          ppEnvironmentVarTable
)
{
    HRESULT hr = S_OK;
    BOOL    fFound = FALSE;
    DWORD   dwResult, dwError;
    STRU    strIisAuthEnvValue;
    STACK_STRU(strStartupAssemblyEnv, 1024);
    ENVIRONMENT_VAR_ENTRY* pHostingEntry = NULL;
    ENVIRONMENT_VAR_ENTRY* pIISAuthEntry = NULL;
    ENVIRONMENT_VAR_HASH* pEnvironmentVarTable = NULL;

    pEnvironmentVarTable = new ENVIRONMENT_VAR_HASH();
    if (pEnvironmentVarTable == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Finished;
    }

    //
    // few environment variables expected, small bucket size for hash table
    //
    if (FAILED(hr = pEnvironmentVarTable->Initialize(37 /*prime*/)))
    {
        goto Finished;
    }

    // copy the envirable hash table (from configuration) to a temp one as we may need to remove elements 
    pInEnvironmentVarTable->Apply(ENVIRONMENT_VAR_HASH::CopyToTable, pEnvironmentVarTable);
    if (pEnvironmentVarTable->Count() != pInEnvironmentVarTable->Count())
    {
        // hash table copy failed
        hr = E_UNEXPECTED;
        goto Finished;
    }

    pEnvironmentVarTable->FindKey(ASPNETCORE_IIS_AUTH_ENV_STR, &pIISAuthEntry);
    if (pIISAuthEntry != NULL)
    {
        // user defined ASPNETCORE_IIS_HTTPAUTH in configuration, wipe it off
        pIISAuthEntry->Dereference();
        pEnvironmentVarTable->DeleteKey(ASPNETCORE_IIS_AUTH_ENV_STR);
    }

    if (fWindowsAuthEnabled)
    {
        strIisAuthEnvValue.Copy(ASPNETCORE_IIS_AUTH_WINDOWS);
    }

    if (fBasicAuthEnabled)
    {
        strIisAuthEnvValue.Append(ASPNETCORE_IIS_AUTH_BASIC);
    }

    if (fAnonymousAuthEnabled)
    {
        strIisAuthEnvValue.Append(ASPNETCORE_IIS_AUTH_ANONYMOUS);
    }

    if (strIisAuthEnvValue.IsEmpty())
    {
        strIisAuthEnvValue.Copy(ASPNETCORE_IIS_AUTH_NONE);
    }

    pIISAuthEntry = new ENVIRONMENT_VAR_ENTRY();
    if (pIISAuthEntry == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Finished;
    }
    if (FAILED(hr = pIISAuthEntry->Initialize(ASPNETCORE_IIS_AUTH_ENV_STR, strIisAuthEnvValue.QueryStr())) ||
        FAILED(hr = pEnvironmentVarTable->InsertRecord(pIISAuthEntry)))
    {
        goto Finished;
    }

    pEnvironmentVarTable->FindKey(HOSTING_STARTUP_ASSEMBLIES_NAME, &pHostingEntry);
    if (pHostingEntry != NULL)
    {
        // user defined ASPNETCORE_HOSTINGSTARTUPASSEMBLIES in configuration
        // the value will be used in OutputEnvironmentVariables. Do nothing here
        pHostingEntry->Dereference();
        pHostingEntry = NULL;
        goto Skipped;
    }

    //check whether ASPNETCORE_HOSTINGSTARTUPASSEMBLIES is defined in system
    dwResult = GetEnvironmentVariable(HOSTING_STARTUP_ASSEMBLIES_ENV_STR,
        strStartupAssemblyEnv.QueryStr(),
        strStartupAssemblyEnv.QuerySizeCCH());
    if (dwResult == 0)
    {
        dwError = GetLastError();
        // Windows API (e.g., CreateProcess) allows variable with empty string value
        // in such case dwResult will be 0 and dwError will also be 0
        // As UI and CMD does not allow empty value, ignore this environment var
        if (dwError != ERROR_ENVVAR_NOT_FOUND && dwError != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(dwError);
            goto Finished;
        }
    }
    else if (dwResult > strStartupAssemblyEnv.QuerySizeCCH())
    {
        // have to increase the buffer and try get environment var again
        strStartupAssemblyEnv.Reset();
        strStartupAssemblyEnv.Resize(dwResult + (DWORD)wcslen(HOSTING_STARTUP_ASSEMBLIES_VALUE) + 1);
        dwResult = GetEnvironmentVariable(HOSTING_STARTUP_ASSEMBLIES_ENV_STR,
            strStartupAssemblyEnv.QueryStr(),
            strStartupAssemblyEnv.QuerySizeCCH());
        if (strStartupAssemblyEnv.IsEmpty())
        {
            hr = E_UNEXPECTED;
            goto Finished;
        }
        fFound = TRUE;
    }
    else
    {
        fFound = TRUE;
    }

    strStartupAssemblyEnv.SyncWithBuffer();
    if (fFound)
    {
        strStartupAssemblyEnv.Append(L";");
    }
    strStartupAssemblyEnv.Append(HOSTING_STARTUP_ASSEMBLIES_VALUE);

    // the environment variable was not defined, create it and add to hashtable
    pHostingEntry = new ENVIRONMENT_VAR_ENTRY();
    if (pHostingEntry == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Finished;
    }
    if (FAILED(hr = pHostingEntry->Initialize(HOSTING_STARTUP_ASSEMBLIES_NAME, strStartupAssemblyEnv.QueryStr())) ||
        FAILED(hr = pEnvironmentVarTable->InsertRecord(pHostingEntry)))
    {
        goto Finished;
    }

Skipped:
    *ppEnvironmentVarTable = pEnvironmentVarTable;
    pEnvironmentVarTable = NULL;

Finished:
    if (pHostingEntry != NULL)
    {
        pHostingEntry->Dereference();
        pHostingEntry = NULL;
    }

    if (pIISAuthEntry != NULL)
    {
        pIISAuthEntry->Dereference();
        pIISAuthEntry = NULL;
    }

    if (pEnvironmentVarTable != NULL)
    {
        pEnvironmentVarTable->Clear();
        delete pEnvironmentVarTable;
        pEnvironmentVarTable = NULL;
    }
    return hr;
}
