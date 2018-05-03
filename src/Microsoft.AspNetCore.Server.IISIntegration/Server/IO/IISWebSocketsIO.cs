// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

using System;
using System.Buffers;
using System.Threading.Tasks;
using Microsoft.Extensions.Logging;

namespace Microsoft.AspNetCore.Server.IISIntegration
{
    internal class WebSocketsAsyncIOEngine: IAsyncIOEngine
    {
        private readonly IntPtr _handler;

        private bool _isInitialized = false;

        private AsyncFlushOperation _initializationFlush;

        public WebSocketsAsyncIOEngine(IntPtr handler)
        {
            _handler = handler;
        }

        public ValueTask Initialize()
        {
            if (_isInitialized)
            {
                throw new InvalidOperationException("Already initialized");
            }

            _initializationFlush = new AsyncFlushOperation();
            _initializationFlush.Initialize(_handler);
            var continuation = _initializationFlush.Invoke();

            if (continuation != null)
            {
                _isInitialized = true;
            }

            return new ValueTask(_initializationFlush, 0);
        }

        public ValueTask<int> ReadAsync(Memory<byte> memory)
        {
            CheckInitialized();

            var read = new WebSocketReadOperation();
            read.Initialize(_handler, memory);
            read.Invoke();
            return new ValueTask<int>(read, 0);
        }

        public ValueTask<int> WriteAsync(ReadOnlySequence<byte> data)
        {
            var read = new WebSocketWriteOperation();
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
            if (_initializationFlush == null)
            {
                throw new InvalidOperationException("Unexpected completion for WebSocket operation");
            }

            var continuation = _initializationFlush.NotifyCompletion(hr, bytes);
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
