using System;
using System.Runtime.InteropServices;
using Microsoft.AspNetCore.HttpSys.Internal;

namespace Microsoft.AspNetCore.Server.IISIntegration
{
    internal class WebSocketWriteOperation : AsyncWriteOperationBase
    {
        private readonly GCHandle _thisHandle;

        public WebSocketWriteOperation()
        {
            _thisHandle = GCHandle.Alloc(this);
        }

        internal override unsafe int WriteChunks(IntPtr requestHandler, int nChunks, HttpApiTypes.HTTP_DATA_CHUNK* pDataChunks, out bool fCompletionExpected)
        {
            return NativeMethods.HttpWebsocketsWriteBytes(requestHandler, pDataChunks, nChunks, WriteCallback, (IntPtr)_thisHandle, out fCompletionExpected);
        }

        private static readonly NativeMethods.PFN_WEBSOCKET_ASYNC_COMPLETION WriteCallback = (IntPtr pHttpContext, IntPtr pCompletionInfo, IntPtr pvCompletionContext) =>
        {
            var context = (WebSocketWriteOperation)GCHandle.FromIntPtr(pvCompletionContext).Target;

            NativeMethods.HttpGetCompletionInfo(pCompletionInfo, out int cbBytes, out int hr);

            context.NotifyCompletion(hr, cbBytes);

            return NativeMethods.REQUEST_NOTIFICATION_STATUS.RQ_NOTIFICATION_PENDING;
        };
    }
}
