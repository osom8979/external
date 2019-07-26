///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier: LGPL-2.1+
//
/// @addtogroup uv
/// @{


#ifndef SCY_UV_Request_H
#define SCY_UV_Request_H


#include "scy/base.h"
#include "scy/loop.h"
#include "scy/util.h"

#include "uv.h"

#include <functional>


namespace scy {
namespace uv {


template <typename T>
class Base_API Handle;


/// Default request callback event.
struct BasicEvent
{
    int status;
};


/// Wrapper class for managing `uv_req_t` variants.
///
/// This class povides safe access to the parent handle incase the handle gets
/// destroyed before the request callback returns, and should be used whenever
/// the handle pointer is accessed via the callback.
template<typename T, typename E = BasicEvent>
struct Request
{
    typedef T Type;
    typedef E Event;

    T req;
    std::function<void(const E&)> callback;

    Request()
    {
        req.data = this;
    }

    static void defaultCallback(T* req, int status)
    {
        auto wrap = static_cast<Request*>(req->data);
        if (wrap->callback)
            wrap->callback(E{status});
        delete wrap;
    }

    template<typename F, typename... Args>
    auto invoke(F&& f, Args&&... args)
    -> std::enable_if_t<!std::is_void<std::result_of_t<F(Args...)>>::value, int>
    {
        auto err = std::forward<F>(f)(std::forward<Args>(args)...);
        if (err && callback) callback(E{err});
        return !err;
    }

    template<typename F, typename... Args>
    auto invoke(F&& f, Args&&... args)
    -> std::enable_if_t<std::is_void<std::result_of_t<F(Args...)>>::value>
    {
        std::forward<F>(f)(std::forward<Args>(args)...);
    }
};


/// Generic helper for instantiating requests.
template<typename T>
inline T& createRequest(std::function<void(const typename T::Event&)> callback)
{
    auto req = new T();
    req->callback = callback;
    return *req;
}


/// Stream connection request for sockets and pipes.
struct ConnectReq : public uv::Request<uv_connect_t>
{
    ConnectReq()
    {
        req.data = this;
    }

    auto connect(uv_tcp_t* handle, const struct sockaddr* addr)
    {
        return invoke(&uv_tcp_connect, &req, handle, addr, &defaultCallback);
    }

    auto connect(uv_pipe_t* handle, const char* name)
    {
        return invoke(&uv_pipe_connect, &req, handle, name, &defaultCallback);
    }
};


/// Get address info request callback event.
struct GetAddrInfoEvent
{
    int status;
    struct addrinfo* addr;
};


/// DNS resolver request to get the IP address of a hostname.
struct GetAddrInfoReq : public uv::Request<uv_getaddrinfo_t, GetAddrInfoEvent>
{
    typedef uv::Request<uv_getaddrinfo_t, GetAddrInfoEvent> Request;

    GetAddrInfoReq()
    {
        req.data = this;
    }

    static void getAddrInfoCallback(Request::Type* req, int status, struct addrinfo* res)
    {
        auto wrap = static_cast<GetAddrInfoReq*>(req->data);
        if (wrap->callback)
            wrap->callback(GetAddrInfoEvent{status, res});
        uv_freeaddrinfo(res);
        delete wrap;
    }

    auto resolve(const std::string& host, int port, uv::Loop* loop = uv::defaultLoop())
    {
        return invoke(&uv_getaddrinfo, loop, &req, &getAddrInfoCallback,
                      host.c_str(), util::itostr(port).c_str(), nullptr);
    }
};




} // namespace uv
} // namespace scy


#endif // SCY_UV_Request


/// @\}
