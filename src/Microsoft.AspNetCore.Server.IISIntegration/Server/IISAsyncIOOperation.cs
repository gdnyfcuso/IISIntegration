using System;
using System.Runtime.InteropServices;
using System.Threading.Tasks.Sources;

namespace Microsoft.AspNetCore.Server.IISIntegration
{
    internal abstract class IISAsyncIOOperation: IValueTaskSource<int>, IValueTaskSource
    {
        private Action<object> _continuation;
        private object _state;
        private int _result;

        private bool _completed;

        private Exception _exception;

        public ValueTaskSourceStatus GetStatus(short token)
        {
            if (!_completed)
            {
                return ValueTaskSourceStatus.Pending;
            }

            return _exception != null ? ValueTaskSourceStatus.Succeeded : ValueTaskSourceStatus.Faulted;
        }

        public void OnCompleted(Action<object> continuation, object state, short token, ValueTaskSourceOnCompletedFlags flags)
        {
            if (_completed)
            {
                continuation(state);
                return;
            }

            if (_continuation != null)
            {
                throw new InvalidOperationException();
            }

            _continuation = continuation;
            _state = state;
        }

        void IValueTaskSource.GetResult(short token)
        {
            if (_exception != null)
            {
                throw _exception;
            }
        }

        public int GetResult(short token)
        {
            if (_exception != null)
            {
                throw _exception;
            }

            return _result;
        }

        public IISAsyncContinuation? Invoke()
        {
            if (InvokeOperation())
            {
                return new IISAsyncContinuation(_continuation, _state);
            }
            return null;
        }

        public abstract bool InvokeOperation();


        public IISAsyncContinuation NotifyCompletion(int hr, int bytes)
        {
            SetResult(hr, bytes);
            NotifyOperationCompletion(hr, bytes);
            return new IISAsyncContinuation(_continuation, _state);
        }

        protected void SetResult(int hr, int bytes)
        {
            _completed = true;
            _result = bytes;
            _exception = Marshal.GetExceptionForHR(hr);
        }

        public abstract void NotifyOperationCompletion(int hr, int bytes);

        public void Reset()
        {
            _exception = null;
            _result = int.MinValue;
            _state = null;
            _continuation = null;
        }

        public abstract void ResetOperation();

        public struct IISAsyncContinuation
        {
            public Action<object> Continuation { get; }
            public object State { get; }

            public IISAsyncContinuation(Action<object> continuation, object state)
            {
                Continuation = continuation;
                State = state;
            }

            public void Invoke()
            {
                Continuation?.Invoke(State);
            }
        }
    }
}