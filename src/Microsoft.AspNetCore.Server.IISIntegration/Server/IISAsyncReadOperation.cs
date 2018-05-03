using System;
using System.Buffers;

namespace Microsoft.AspNetCore.Server.IISIntegration
{
    internal class IISAsyncReadOperation : IISAsyncIOOperation
    {
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
            var hr = NativeMethods.HttpReadRequestBytes(
                _requestHandler,
                (byte*)_inputHandle.Pointer,
                _memory.Length,
                out var dwReceivedBytes,
                out bool fCompletionExpected);

            if (!fCompletionExpected)
            {
                SetResult(hr, dwReceivedBytes);
                NotifyOperationCompletion(hr, dwReceivedBytes);
                return true;
            }

            return false;
        }

        public override void ResetOperation()
        {
            _memory = default;
            _inputHandle.Dispose();
            _inputHandle = default;
            _requestHandler = default;
        }

        public override void NotifyOperationCompletion(int hr, int bytes)
        {
            _inputHandle.Dispose();
        }
    }
}