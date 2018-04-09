// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

using System;
using System.IO;
using System.Net;
using System.Threading;
using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Http.Features;
using Microsoft.AspNetCore.Http.Features.Authentication;
using Microsoft.Extensions.Logging;
using Xunit;

namespace IISTestSite
{
    public class StartupFeatureCollection
    {
        public void Configure(IApplicationBuilder app, ILoggerFactory loggerFactory)
        {
            app.Run(async context =>
            {
                try
                {
                    // Verify setting and getting each feature/ portion of the httpcontext works

                }
                catch (Exception exception)
                {
                    context.Response.StatusCode = 500;
                    await context.Response.WriteAsync(exception.ToString());
                }
                await context.Response.WriteAsync("_Failure");
            });
        }
    }
}
