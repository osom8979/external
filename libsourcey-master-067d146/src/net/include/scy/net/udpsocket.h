///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier: LGPL-2.1+
//
/// @addtogroup net
/// @{


#ifndef SCY_Net_UDPSocket_H
#define SCY_Net_UDPSocket_H


#include "scy/net/address.h"
#include "scy/net/socket.h"
#include "scy/net/net.h"
#include "scy/handle.h"


namespace scy {
namespace net {


/// UDP socket implementation.
class Net_API UDPSocket : public uv::Handle<uv_udp_t>, public net::Socket
{
public:
    typedef std::shared_ptr<UDPSocket> Ptr;
    typedef std::vector<Ptr> Vec;

    UDPSocket(uv::Loop* loop = uv::defaultLoop());
    virtual ~UDPSocket();

    virtual void connect(const net::Address& peerAddress) override;
    virtual void connect(const std::string& host, uint16_t port) override;
    virtual void close() override;

    virtual void bind(const net::Address& address, unsigned flags = 0) override;

    virtual ssize_t send(const char* data, size_t len, int flags = 0) override;
    virtual ssize_t send(const char* data, size_t len,
                         const net::Address& peerAddress, int flags = 0) override;

    bool setBroadcast(bool flag);
    bool setMulticastLoop(bool flag);
    bool setMulticastTTL(int ttl);

    virtual net::Address address() const override;
    virtual net::Address peerAddress() const override;

    /// Returns the UDP transport protocol.
    net::TransportType transport() const override;

    virtual void setError(const scy::Error& err) override;
    virtual const scy::Error& error() const override;

    /// Returns true if the native socket handle is closed.
    virtual bool closed() const override;

    virtual uv::Loop* loop() const override;

    virtual void* self() override;

    virtual void onRecv(const MutableBuffer& buf, const net::Address& address);

protected:
    virtual void init() override;
    virtual void reset() override;

    virtual void onError(const scy::Error& error) override;
    virtual void onClose() override;

    virtual bool recvStart();
    virtual bool recvStop();

    static void onRecv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf,
                       const struct sockaddr* addr, unsigned flags);
    static void allocRecvBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);

    net::Address _peer;
    Buffer _buffer;
};


} // namespace net
} // namespace scy


#endif // SCY_Net_UDPSocket_H


/// @\}
