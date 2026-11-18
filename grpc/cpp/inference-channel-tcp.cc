/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
// Copyright (c) 2026 Muhammad Uzair
// SPDX-License-Identifier: GPL-2.0-only

#include "inference-channel-tcp.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace ns3
{
namespace oranntn
{
namespace airan
{

namespace
{

void
SetNagleOff(int fd)
{
    int one = 1;
    ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
}

void
SetReuseAddr(int fd)
{
    int one = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
}

bool
WaitForReadable(int fd, uint32_t timeout_ms)
{
    struct pollfd pfd
    {
    };
    pfd.fd = fd;
    pfd.events = POLLIN;
    const int rc = ::poll(&pfd, 1, static_cast<int>(timeout_ms));
    if (rc <= 0)
    {
        return false;
    }
    return (pfd.revents & POLLIN) != 0;
}

} // namespace

TcpInferenceChannel::TcpInferenceChannel() = default;

TcpInferenceChannel::TcpInferenceChannel(int fd)
    : m_fd(fd)
{
    if (m_fd >= 0)
    {
        SetNagleOff(m_fd);
    }
}

TcpInferenceChannel::~TcpInferenceChannel()
{
    Close();
}

bool
TcpInferenceChannel::ConnectTo(const std::string& host, uint16_t port)
{
    Close();
    struct addrinfo hints
    {
    };
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo* res = nullptr;
    const std::string portstr = std::to_string(port);
    const std::string h = host.empty() ? "127.0.0.1" : host;
    if (::getaddrinfo(h.c_str(), portstr.c_str(), &hints, &res) != 0 ||
        !res)
    {
        return false;
    }
    int fd = ::socket(res->ai_family,
                      res->ai_socktype,
                      res->ai_protocol);
    if (fd < 0)
    {
        ::freeaddrinfo(res);
        return false;
    }
    if (::connect(fd, res->ai_addr, res->ai_addrlen) != 0)
    {
        ::close(fd);
        ::freeaddrinfo(res);
        return false;
    }
    ::freeaddrinfo(res);
    SetNagleOff(fd);
    m_fd = fd;
    return true;
}

bool
TcpInferenceChannel::Send(const std::vector<uint8_t>& bytes)
{
    if (m_fd < 0)
    {
        return false;
    }
    // The receiver closes the connection on an empty or oversized frame, so
    // refuse to send one rather than lose the channel on the far side.
    if (bytes.empty() || bytes.size() > kMaxFrameBytes)
    {
        return false;
    }
    const uint32_t len_be = htonl(static_cast<uint32_t>(bytes.size()));
    if (::send(m_fd, &len_be, sizeof(len_be), MSG_NOSIGNAL) !=
        static_cast<ssize_t>(sizeof(len_be)))
    {
        Close();
        return false;
    }
    const uint8_t* p = bytes.data();
    size_t remaining = bytes.size();
    while (remaining > 0)
    {
        const ssize_t n = ::send(m_fd, p, remaining, MSG_NOSIGNAL);
        if (n <= 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            Close();
            return false;
        }
        p += n;
        remaining -= static_cast<size_t>(n);
    }
    m_bytesSent.fetch_add(bytes.size());
    m_framesSent.fetch_add(1);
    return true;
}

bool
TcpInferenceChannel::RecvExact(uint8_t* out,
                                size_t n,
                                uint32_t timeout_ms)
{
    size_t got = 0;
    while (got < n)
    {
        if (!WaitForReadable(m_fd, timeout_ms))
        {
            return false;
        }
        const ssize_t r = ::recv(m_fd, out + got, n - got, 0);
        if (r <= 0)
        {
            if (r < 0 && errno == EINTR)
            {
                continue;
            }
            Close();
            return false;
        }
        got += static_cast<size_t>(r);
    }
    return true;
}

bool
TcpInferenceChannel::TryRecv(std::vector<uint8_t>& bytes,
                              uint32_t timeout_ms)
{
    // Frames are reassembled in m_rxStash across calls. This used to read the
    // 4-byte length and then the payload with two RecvExact calls; when the
    // poll timed out between or inside them (Poll(0) from a simulator tick is
    // the normal case, and a 100 kB CSI frame arrives in several segments) the
    // bytes already read were dropped, and the next call parsed the middle of
    // a frame as a length header. The stash member existed for exactly this
    // and was never used.
    for (;;)
    {
        if (m_rxStash.size() >= sizeof(uint32_t))
        {
            uint32_t len_be = 0;
            std::memcpy(&len_be, m_rxStash.data(), sizeof(len_be));
            const uint32_t len = ntohl(len_be);
            if (len == 0 || len > kMaxFrameBytes)
            {
                m_rxStash.clear();
                Close();
                return false;
            }
            if (m_rxStash.size() >= sizeof(uint32_t) + len)
            {
                bytes.assign(m_rxStash.begin() + sizeof(uint32_t),
                             m_rxStash.begin() + sizeof(uint32_t) + len);
                m_rxStash.erase(m_rxStash.begin(),
                                m_rxStash.begin() + sizeof(uint32_t) + len);
                m_bytesRecv.fetch_add(len);
                m_framesRecv.fetch_add(1);
                return true;
            }
        }
        if (m_fd < 0 || !WaitForReadable(m_fd, timeout_ms))
        {
            return false;
        }
        uint8_t chunk[65536];
        const ssize_t r = ::recv(m_fd, chunk, sizeof(chunk), 0);
        if (r <= 0)
        {
            if (r < 0 && errno == EINTR)
            {
                continue;
            }
            if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                return false;
            }
            Close();
            return false;
        }
        m_rxStash.insert(m_rxStash.end(), chunk, chunk + r);
    }
}

bool
TcpInferenceChannel::IsOpen() const
{
    return m_fd >= 0;
}

void
TcpInferenceChannel::Close()
{
    if (m_fd >= 0)
    {
        ::shutdown(m_fd, SHUT_RDWR);
        ::close(m_fd);
        m_fd = -1;
    }
}

// ----------------------------------------------------------------------------
// TcpInferenceListener
// ----------------------------------------------------------------------------

TcpInferenceListener::TcpInferenceListener() = default;

TcpInferenceListener::~TcpInferenceListener()
{
    Close();
}

bool
TcpInferenceListener::Listen(const std::string& host,
                              uint16_t port,
                              int backlog)
{
    Close();
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        return false;
    }
    SetReuseAddr(fd);

    struct sockaddr_in addr
    {
    };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (host.empty() || host == "0.0.0.0")
    {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else if (host == "127.0.0.1" || host == "localhost")
    {
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }
    else
    {
        if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1)
        {
            ::close(fd);
            return false;
        }
    }
    if (::bind(fd,
               reinterpret_cast<struct sockaddr*>(&addr),
               sizeof(addr)) != 0)
    {
        ::close(fd);
        return false;
    }
    if (::listen(fd, backlog) != 0)
    {
        ::close(fd);
        return false;
    }
    // Read back the resolved port (port=0 → ephemeral).
    socklen_t alen = sizeof(addr);
    if (::getsockname(fd,
                      reinterpret_cast<struct sockaddr*>(&addr),
                      &alen) == 0)
    {
        m_port = ntohs(addr.sin_port);
    }
    else
    {
        m_port = port;
    }
    m_fd = fd;
    return true;
}

std::unique_ptr<TcpInferenceChannel>
TcpInferenceListener::AcceptOne(uint32_t timeout_ms)
{
    if (m_fd < 0)
    {
        return nullptr;
    }
    if (!WaitForReadable(m_fd, timeout_ms))
    {
        return nullptr;
    }
    struct sockaddr_in addr
    {
    };
    socklen_t alen = sizeof(addr);
    const int cfd = ::accept(m_fd,
                             reinterpret_cast<struct sockaddr*>(&addr),
                             &alen);
    if (cfd < 0)
    {
        return nullptr;
    }
    return std::unique_ptr<TcpInferenceChannel>(
        new TcpInferenceChannel(cfd));
}

void
TcpInferenceListener::Close()
{
    if (m_fd >= 0)
    {
        ::close(m_fd);
        m_fd = -1;
    }
    m_port = 0;
}

} // namespace airan
} // namespace oranntn
} // namespace ns3
