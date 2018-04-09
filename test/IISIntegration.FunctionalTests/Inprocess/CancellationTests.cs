// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

using System.Net.Http;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Testing.xunit;
using Xunit;

namespace Microsoft.AspNetCore.Server.IISIntegration.FunctionalTests
{
    [Collection(IISTestSiteCollection.Name)]
    public class CancellationTests
    {
        private readonly IISTestSiteFixture _fixture;

        public CancellationTests(IISTestSiteFixture fixture)
        {
            _fixture = fixture;
        }

        [Fact]
        public void CanCancelRequestOnServer()
        {
            // TODO currently, failed request return a 200 status code. Filed issue: https://github.com/aspnet/IISIntegration/issues/591
            var responseString = RequestUtilities.SendHungHttpPostRequest(_fixture.Client.BaseAddress, "/CancelRequest");
            Assert.DoesNotContain("hello world", responseString);
        }
    }
}
