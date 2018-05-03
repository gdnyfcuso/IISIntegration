using System;
using System.Buffers;
using System.Threading.Tasks;

namespace Microsoft.AspNetCore.Server.IISIntegration
{
    internal interface IIISIO
    {
        ValueTask<int> ReadAsync(Memory<byte> memory);
        ValueTask<int> WriteAsync(ReadOnlySequence<byte> data);
        ValueTask FlushAsync();
        void NotifyCompletion(int hr, int bytes);
        void Stop();
    }
}