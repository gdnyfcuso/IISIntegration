// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

using System;
using System.Buffers;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Microsoft.AspNetCore.HttpSys.Internal;
using Microsoft.Extensions.Logging;

namespace Microsoft.AspNetCore.Server.IISIntegration
{
    internal class IISWebSocketWriteOperation : IISAsyncWriteOperationBase
    {
        private readonly GCHandle _thisHandle;

        public IISWebSocketWriteOperation()
        {
            _thisHandle = GCHandle.Alloc(this);
        }

        internal override unsafe int WriteChunks(IntPtr requestHandler, int nChunks, HttpApiTypes.HTTP_DATA_CHUNK* pDataChunks, out bool fCompletionExpected)
        {
            return NativeMethods.HttpWebsocketsWriteBytes(requestHandler, pDataChunks, nChunks, WriteCallback, (IntPtr)_thisHandle, out fCompletionExpected);
        }

        private static readonly NativeMethods.PFN_WEBSOCKET_ASYNC_COMPLETION WriteCallback = (IntPtr pHttpContext, IntPtr pCompletionInfo, IntPtr pvCompletionContext) =>
        {
            var context = (IISWebSocketWriteOperation)GCHandle.FromIntPtr(pvCompletionContext).Target;

            NativeMethods.HttpGetCompletionInfo(pCompletionInfo, out int cbBytes, out int hr);

            context.NotifyCompletion(hr, cbBytes);

            return NativeMethods.REQUEST_NOTIFICATION_STATUS.RQ_NOTIFICATION_PENDING;
        };
    }
    internal class IISWebSocketReadOperation : IISAsyncIOOperation
    {
        private readonly GCHandle _thisHandle;

        public IISWebSocketReadOperation()
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
            var context = (IISWebSocketReadOperation)GCHandle.FromIntPtr(pvCompletionContext).Target;

            NativeMethods.HttpGetCompletionInfo(pCompletionInfo, out int cbBytes, out int hr);

            var continuation = context.NotifyCompletion(hr, cbBytes);

            continuation.Invoke();

            return NativeMethods.REQUEST_NOTIFICATION_STATUS.RQ_NOTIFICATION_PENDING;
        };
    }

    internal class IISWebSocketsIO: IIISIO
    {
        private readonly IntPtr _handler;

        private bool _isInitialized = false;

        private IISAsyncFlushOperation _flush;

        public IISWebSocketsIO(IntPtr handler)
        {
            _handler = handler;
        }

        public ValueTask Initialize()
        {
            if (_isInitialized)
            {
                throw new InvalidOperationException("Already initialized");
            }

            _flush = new IISAsyncFlushOperation();
            _flush.Initialize(_handler);
            var continuation = _flush.Invoke();
            if (continuation != null)
            {
                _isInitialized = true;
            }
            return new ValueTask(_flush, 0);
        }

        public ValueTask<int> ReadAsync(Memory<byte> memory)
        {
            CheckInitialized();

            var read = new IISWebSocketReadOperation();
            read.Initialize(_handler, memory);
            read.Invoke();
            return new ValueTask<int>(read, 0);
        }

        public ValueTask<int> WriteAsync(ReadOnlySequence<byte> data)
        {
            var read = new IISWebSocketWriteOperation();
            read.Initialize(_handler, data);
            read.Invoke();
            return new ValueTask<int>(read, 0);
        }

        public ValueTask FlushAsync()
        {
            // WebSockets auto flush
            return new ValueTask(Task.CompletedTask);
        }

        public void NotifyCompletion(int hr, int bytes)
        {
            _isInitialized = true;
            if (_flush == null)
            {
                throw new InvalidOperationException("Unexpected completion for WebSocket operation");
            }

            var continuation = _flush.NotifyCompletion(hr, bytes);
            continuation.Invoke();
        }

        private void CheckInitialized()
        {
            if (!_isInitialized)
            {
                throw new InvalidOperationException("IO not initialized yet");
            }
        }

        public void Stop()
        {
            // TODO
        }
    }
}
