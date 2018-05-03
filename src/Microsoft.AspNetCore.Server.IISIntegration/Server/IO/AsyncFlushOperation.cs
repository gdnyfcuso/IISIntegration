using System;

namespace Microsoft.AspNetCore.Server.IISIntegration
{
    internal class AsyncFlushOperation: AsyncIOOperation
    {
        private IntPtr _requestHandler;

        public void Initialize(IntPtr requestHandler)
        {
            _requestHandler = requestHandler;
        }

        public override bool InvokeOperation()
        {
            var hr = NativeMethods.HttpFlushResponseBytes(_requestHandler,  out var fCompletionExpected);
            if (!fCompletionExpected)
            {
                SetResult(hr, 0);
                return true;
            }

            return false;
        }

        public override void NotifyOperationCompletion(int hr, int bytes)
        {
        }

        public override void ResetOperation()
        {
            _requestHandler = default;
        }
    }
}
