///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier: LGPL-2.1+
//
/// @addtogroup base
/// @{


#ifndef SCY_Runner_H
#define SCY_Runner_H


#include "scy/base.h"
#include "scy/interface.h"
#include "scy/platform.h"
#include "scy/util.h"

#include <cstdint>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>
#include <stdexcept>
#include <iostream>


namespace scy {


/// Runner is a virtual interface for implementing
/// asynchronous objects such as threads and futures.
class Base_API Runner
{
public:
    Runner();
    virtual ~Runner();

    /// Start the asynchronous context with the given invokeback.
    ///
    /// The target `Runnable` instance must outlive the `Runner`.
    virtual void start(std::function<void()> target) = 0;

    /// Returns true if the async context is currently running.
    bool running() const;

    /// Cancels the async context.
    void cancel();

    /// True when the task has been cancelled.
    /// It is up to the implementation to return at the
    /// earliest possible time.
    bool cancelled() const;

    /// Returns true if the Runner is operating in repeating mode.
    bool repeating() const;

    /// This setting means the implementation should invoke the
    /// target function repeatedly until cancelled. The importance
    /// of this method to normalize the functionality of threadded
    /// and event loop driven Runner models.
    void setRepeating(bool flag);

    /// Returns true if the implementation is thread-based, or false
    /// if it belongs to an event loop.
    virtual bool async() const = 0;

    /// Return the native thread ID.
    std::thread::id tid() const;

    /// Wait until the thread exits.
    ///
    /// The thread should be cancelled beore invokeing this method.
    /// This method must be invokeed from outside the current thread
    /// context or deadlock will ensue.
    bool waitForExit(int timeout = 5000);

    /// Context object which we send to the thread context.
    ///
    /// This intermediate object allows us to garecefully handle late invokebacks
    /// and so avoid the need for deferred destruction of `Runner` objects.
    struct Context
    {
        std::thread::id tid;
        std::atomic<bool> running;
        std::atomic<bool> cancelled;
        bool repeating = false;

        // The implementation is responsible for resetting
        // the context if it is to be reused.
        void reset()
        {
            running = false;
            cancelled = false;
        }

        Context()
        {
            reset();
        }
    };

protected:
    /// NonCopyable and NonMovable
    Runner(const Runner&) = delete;
    Runner& operator=(const Runner&) = delete;

    /// Shared pointer to the internal Context.
    std::shared_ptr<Context> _context;
};


//
// Internal Helpers
//


namespace internal {


/// Helper function for running an async context.
template<typename Function, typename... Args>
inline void runAsync(std::shared_ptr<Runner::Context> c, Function func, Args... args)
{
    // std::cout << "Runner::runAsync" << std::endl;
    c->tid = std::this_thread::get_id();
    c->running = true;
    do {
#ifdef SCY_EXCEPTION_RECOVERY
        try {
#endif
            func(std::forward<Args>(args)...);
#ifdef SCY_EXCEPTION_RECOVERY
        } catch (std::exception& exc) {
            std::cerr << "Runner exception: " << exc.what() << std::endl;
        }
#endif
        scy::sleep(1);
    } while (c->repeating && !c->cancelled);
    c->running = false;
}


/// Call a function with the given argument tuple.
///
/// Note: This will become redundant once C++17 `std::apply` is fully supported.
template<typename Function, typename Tuple, size_t ... I>
auto invoke(Function f, Tuple t, std::index_sequence<I ...>)
{
     return f(std::get<I>(t)...);
}


/// Call a function with the given argument tuple.
///
/// Create an index sequence for the array, and pass it to the
/// implementation `invoke` function.
///
/// Note: This will become redundant once C++17 `std::apply` is fully supported.
template<typename Function, typename Tuple>
auto invoke(Function f, Tuple t)
{
    static constexpr auto size = std::tuple_size<Tuple>::value;
    return invoke(f, t, std::make_index_sequence<size>{});
}


/// Helper class that stores a function pointer and veradic arguments as a tuple
/// for deferred invocation.
template<typename Function, typename... Args>
struct DeferredCallable
{
    std::shared_ptr<Runner::Context> ctx;
    Function func;
    std::tuple<Args...> args;

    DeferredCallable(std::shared_ptr<Runner::Context> c, Function&& f, Args&&... a)
        : ctx(c)
        , func(f)
        , args(std::make_tuple(a...))
    {
    }

    void invoke()
    {
        internal::invoke(func, args);
    }
};


} // namespace internal
} // namespace scy


#endif // SCY_Runner_H


/// @\}
