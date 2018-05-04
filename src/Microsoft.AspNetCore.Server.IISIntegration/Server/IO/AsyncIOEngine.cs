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
            _writeQueue = new Queue<AsyncIOOperation>();
            _readQueue = new Queue<AsyncIOOperation>();
        }

        private readonly Queue<AsyncIOOperation> _writeQueue;
        private readonly Queue<AsyncIOOperation> _readQueue;

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
            lock (_readQueue)
            {
                if (_runningOperation != null)
                {
                    Enqueue(ioOperation);

                    _logger.LogTrace("Operation {Operation} added to queue", ioOperation.GetType());
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

        private void Enqueue(AsyncIOOperation ioOperation)
        {
            if (ioOperation is AsyncReadOperation)
            {
                _readQueue.Enqueue(ioOperation);
            }
            else
            {
                _writeQueue.Enqueue(ioOperation);
            }
        }

        private void CancelPendingRead()
        {
            lock (_readQueue)
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

            lock (_readQueue)
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

            lock (_readQueue)
            {
                if (_readQueue.Count > 0)
                {
                    var ioOperation = _readQueue.Dequeue();

                    continuation = ioOperation.Invoke();
                    // operation went async
                    if (continuation == null)
                    {
                        _runningOperation = ioOperation;
                    }
                }
                else
                {
                    return false;
                }
            }

            continuation?.Invoke();
            return continuation != null;
        }

        public void Stop()
        {
            lock (_readQueue)
            {
                if (_runningOperation != null || _readQueue.Count > 0)
                {
                    throw new InvalidOperationException();
                }
            }
        }

        private bool TryDequeue(out AsyncIOOperation ioOperation)
        {
            if (_readQueue?.Count > 0)
            {
                ioOperation = _readQueue.Dequeue();
                return true;
            }

            if (_writeQueue?.Count > 0)
            {
                ioOperation = _writeQueue.Dequeue();
                return true;
            }

            ioOperation = null;
            return false;
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
