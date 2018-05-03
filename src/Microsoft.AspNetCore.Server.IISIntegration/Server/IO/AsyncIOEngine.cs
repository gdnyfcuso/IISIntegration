using System;
using System.Buffers;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading.Tasks;
using Microsoft.Extensions.Logging;

namespace Microsoft.AspNetCore.Server.IISIntegration
{
    internal class AsyncIOEngine : IAsyncIOEngine
    {
        private readonly IntPtr _handler;

        private readonly ILogger<AsyncIOEngine> _logger;

        public AsyncIOEngine(IntPtr handler, ILogger<AsyncIOEngine> logger)
        {
            _handler = handler;
            _logger = logger;
            _operationQueue = new Queue<AsyncIOOperation>();
        }

        private readonly Queue<AsyncIOOperation> _operationQueue;

        private AsyncIOOperation _runningOperation;

        public ValueTask<int> ReadAsync(Memory<byte> memory)
        {
            var read = GetReadOperation();
            read.Initialize(_handler, memory);
            Run(read);
            return new ValueTask<int>(read, 0);
        }

        private AsyncReadOperation GetReadOperation()
        {
            return new AsyncReadOperation();
        }

        private AsyncWriteOperation GetWriteOperation()
        {
            return new AsyncWriteOperation();
        }
        private AsyncFlushOperation GetFlushOperation()
        {
            return new AsyncFlushOperation();
        }
        public ValueTask<int> WriteAsync(ReadOnlySequence<byte> data)
        {
            var write = GetWriteOperation();
            write.Initialize(_handler, data);
            Run(write);
            CancelPendingRead();
            return new ValueTask<int>(write, 0);
        }

        private void Run(AsyncIOOperation ioOperation)
        {
            lock (_operationQueue)
            {
                if (_runningOperation != null)
                {
                    _operationQueue.Enqueue(ioOperation);

                    _logger.LogTrace("Operation {Operation} added to queue", ioOperation.GetType());

                }
                else
                {
                    var completed = ioOperation.Invoke() != null;
                    // operation went async
                    if (!completed)
                    {

                        _logger.LogTrace("Operation {Operation} went async", ioOperation.GetType());
                        _runningOperation = ioOperation;
                    }
                    else
                    {
                        _logger.LogTrace("Operation {Operation} competed synchronously", ioOperation.GetType());
                    }
                }
            }
        }

        private void CancelPendingRead()
        {
            lock (_operationQueue)
            {
                if (_runningOperation is AsyncReadOperation)
                {
                    NativeMethods.HttpTryCancelIO(_handler);
                }
            }
        }

        public ValueTask FlushAsync()
        {
            var flush = GetFlushOperation();
            flush.Initialize(_handler);
            Run(flush);
            return new ValueTask(flush, 0);
        }

        public void NotifyCompletion(int hr, int bytes)
        {
            AsyncIOOperation.IISAsyncContinuation continuation;

            lock (_operationQueue)
            {
                Debug.Assert(_runningOperation != null);

                _logger.LogTrace("Got notify completion for {Operation}", _runningOperation.GetType());

                continuation = _runningOperation.NotifyCompletion(hr, bytes);
            }

            continuation.Invoke();

            DrainQueue();
        }

        private void DrainQueue()
        {
            while (RunNextOperation())
            {
            }
        }

        private bool RunNextOperation()
        {
            AsyncIOOperation.IISAsyncContinuation? continuation = null;

            lock (_operationQueue)
            {
                if (_operationQueue.Count > 0)
                {
                    var ioOperation = _operationQueue.Dequeue();

                    continuation = ioOperation.Invoke();
                    // operation went async
                    if (continuation == null)
                    {
                        _runningOperation = ioOperation;
                    }
                }
            }

            continuation?.Invoke();
            return continuation != null;
        }

        public void Stop()
        {
            lock (_operationQueue)
            {
                if (_runningOperation != null || _operationQueue.Count > 0)
                {
                    throw new InvalidOperationException();
                }
            }
        }
    }
}
