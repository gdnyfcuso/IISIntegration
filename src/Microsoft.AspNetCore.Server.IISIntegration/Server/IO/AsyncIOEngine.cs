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

        public AsyncIOEngine(IntPtr handler, ILoggerFactory loggerFactory)
        {
            _handler = handler;
            _logger = loggerFactory.CreateLogger<AsyncIOEngine>();
        }

        private object _syncRoot = new object();
        private AsyncIOOperation _nextOperation;
        private AsyncIOOperation _runningOperation;

        public ValueTask<int> ReadAsync(Memory<byte> memory)
        {
            var read = GetReadOperation();
            read.Initialize(_handler, memory);
            Run(read);
            return new ValueTask<int>(read, 0);
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
            lock (_syncRoot)
            {
                if (_runningOperation != null)
                {
                    if (_nextOperation == null)
                    {
                        _logger.LogTrace("Operation {Operation} added to queue", ioOperation.GetType());
                        _nextOperation = ioOperation;
                    }
                    else
                    {
                        throw new InvalidOperationException("Only one queued operation is allowed");
                    }
                }
                else
                {
                    // we are just starting operation so there would be no
                    // continuation registered
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
            lock (_syncRoot)
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
            AsyncIOOperation.IISAsyncContinuation? nextContinuation = null;

            lock (_syncRoot)
            {
                Debug.Assert(_runningOperation != null);

                _logger.LogTrace("Got notify completion for {Operation}", _runningOperation.GetType());

                continuation = _runningOperation.NotifyCompletion(hr, bytes);

                var next = _nextOperation;
                _nextOperation = null;
                _runningOperation = null;

                if (next != null)
                {
                    nextContinuation = next.Invoke();

                    // operation went async
                    if (nextContinuation == null)
                    {
                        _logger.LogTrace("Next operation {Operation} went async", next.GetType());
                        _runningOperation = next;
                    }
                    else
                    {
                        _logger.LogTrace("Next operation {Operation} competed synchronously", next.GetType());
                    }
                }
            }

            continuation.Invoke();
            nextContinuation?.Invoke();
        }

        public void Stop()
        {
            lock (_syncRoot)
            {
                if (_runningOperation != null || _nextOperation != null)
                {
                    throw new InvalidOperationException();
                }
            }
        }

        protected virtual AsyncReadOperation GetReadOperation()
        {
            return new AsyncReadOperation();
        }

        protected virtual AsyncWriteOperation GetWriteOperation()
        {
            return new AsyncWriteOperation();
        }

        protected virtual AsyncFlushOperation GetFlushOperation()
        {
            return new AsyncFlushOperation();
        }
    }
}
