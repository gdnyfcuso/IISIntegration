using System;
using System.Buffers;
using System.Runtime.InteropServices;

namespace Microsoft.AspNetCore.Server.IISIntegration
{
    internal class WebSocketReadOperation : AsyncIOOperation
    {
        private readonly GCHandle _thisHandle;

        public WebSocketReadOperation()
        {
            _thisHandle = GCHandle.Alloc(this);
        }

        public void Initialize(IntPtr requestHandler, Memory<byte> memory)
        {
            _requestHandler = requestHandler;
            _memory = memory;
        }

        private MemoryHandle _inputHandle;

        private IntPtr _requestHandler;

        private Memory<byte> _memory;

        public override unsafe bool InvokeOperation()
        {
            _inputHandle = _memory.Pin();

            var hr = NativeMethods.HttpWebsocketsReadBytes(
                _requestHandler,
                (byte*)_inputHandle.Pointer,
                _memory.Length,
                ReadCallback,
                (IntPtr)_thisHandle,
                out var dwReceivedBytes,
                out var fCompletionExpected);

            if (!fCompletionExpected)
            {
                SetResult(hr, dwReceivedBytes);
                NotifyOperationCompletion(hr, dwReceivedBytes);
                return true;
            }

            return false;
        }

        public override void NotifyOperationCompletion(int hr, int bytes)
        {
            _inputHandle.Dispose();
        }

        public override void ResetOperation()
        {
            _memory = default;
            _inputHandle.Dispose();
            _inputHandle = default;
            _requestHandler = default;
        }

        public static readonly NativeMethods.PFN_WEBSOCKET_ASYNC_COMPLETION ReadCallback = (pHttpContext, pCompletionInfo, pvCompletionContext) =>
        {
            var context = (WebSocketReadOperation)GCHandle.FromIntPtr(pvCompletionContext).Target;

            NativeMethods.HttpGetCompletionInfo(pCompletionInfo, out int cbBytes, out int hr);

            var continuation = context.NotifyCompletion(hr, cbBytes);

            continuation.Invoke();

            return NativeMethods.REQUEST_NOTIFICATION_STATUS.RQ_NOTIFICATION_PENDING;
        };
    }
}
