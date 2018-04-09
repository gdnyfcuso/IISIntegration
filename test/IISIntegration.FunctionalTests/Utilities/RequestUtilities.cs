using System;
using System.Net;
using System.Net.Http;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Testing.xunit;
using Xunit;

namespace Microsoft.AspNetCore.Server.IISIntegration.FunctionalTests
{
    public static class RequestUtilities
    {
        public static string SendHungHttpPostRequest(Uri uri, string path)
        {
            var ipHostEntry = Dns.GetHostEntry(uri.Host);

            using (var socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp))
            {
                foreach (var hostEntry in ipHostEntry.AddressList)
                {
                    try
                    {
                        socket.Connect(hostEntry, uri.Port);
                        break;
                    }
                    catch (Exception)
                    {
                        // Exceptions can be thrown if the first DNS entry is ipv6
                    }
                }
                if (!socket.Connected)
                {
                    return "";
                }
                
                var request = $"POST {path} HTTP/1.0\r\n" +
                    $"Content-Length: 10\r\n" +
                    "Host: " + "localhost\r\n" +
                    "\r\n";

                var requestBytes = Encoding.ASCII.GetBytes(request);

                var bytesSent = 0;
                while ((bytesSent += socket.Send(requestBytes, bytesSent, 1, SocketFlags.None)) < requestBytes.Length)
                {
                }

                var stringBuilder = new StringBuilder();
                var buffer = new byte[4096];
                int bytesReceived;
                while ((bytesReceived = socket.Receive(buffer, buffer.Length, SocketFlags.None)) != 0)
                {
                    stringBuilder.Append(Encoding.ASCII.GetString(buffer, 0, bytesReceived));
                }

                return stringBuilder.ToString();
            }
        }
    }
}
