// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

using System;
using System.Diagnostics;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Http;
using Microsoft.Extensions.Logging;
using Xunit;

namespace IISTestSite
{
    public class StartupClientDisconnect
    {
        public void Configure(IApplicationBuilder app, ILoggerFactory loggerFactory)
        {
           
        }
    }
}
