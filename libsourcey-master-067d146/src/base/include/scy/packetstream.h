///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier: LGPL-2.1+
//
/// @addtogroup base
/// @{


#ifndef SCY_PacketStream_H
#define SCY_PacketStream_H


#include "scy/base.h"
#include "scy/error.h"
#include "scy/interface.h"
#include "scy/memory.h"
#include "scy/logger.h"
#include "scy/packetsignal.h"
#include "scy/stateful.h"
#include <cstdint>


namespace scy {


struct PacketStreamState;


//
// Packet Stream Adapter
//


/// This class is a wrapper for integrating external
/// classes with the a PacketStream's data flow and
/// state machine.
class Base_API PacketStreamAdapter
{
public:
    PacketStreamAdapter(PacketSignal& emitter);
    virtual ~PacketStreamAdapter(){};

    virtual void emit(char* data, size_t len, unsigned flags = 0);
    virtual void emit(const char* data, size_t len, unsigned flags = 0);
    virtual void emit(const std::string& str, unsigned flags = 0);
    virtual void emit(unsigned flags = 0);
    virtual void emit(IPacket& packet);

    /// Returns a reference to the outgoing packet signal.
    PacketSignal& getEmitter();

    /// Called by the PacketStream to notify when the internal
    /// Stream state changes.
    /// On receiving the Stopped state, it is the responsibility
    /// of the adapter to have ceased all outgoing packet transmission,
    /// especially in multi-thread scenarios.
    virtual void onStreamStateChange(const PacketStreamState&){};

protected:
    /// NonCopyable and NonMovable
    PacketStreamAdapter(const PacketStreamAdapter&) = delete;
    PacketStreamAdapter& operator=(const PacketStreamAdapter&) = delete;

    PacketSignal& _emitter;
};


typedef PacketStreamAdapter PacketSource; ///< For 0.8.x compatibility


//
// PacketProcessor
//

/// This class is a virtual interface for creating
/// PacketStreamAdapters which process that and emit
/// the IPacket type.
class Base_API PacketProcessor : public PacketStreamAdapter
{
public:
    PacketProcessor(PacketSignal& emitter)
        : PacketStreamAdapter(emitter)
    {
    }

    /// This method performs processing on the given
    /// packet and emits the result.
    ///
    /// Note: If packet processing is async (the packet is not in
    /// the current thread scope) then packet data must be copied.
    /// Copied data can be freed directly aFter the async call to
    /// emit() the outgoing packet.
    virtual void process(IPacket& packet) = 0;

    /// This method ensures compatibility with the given
    /// packet type. Return false to reject the packet.
    virtual bool accepts(IPacket*) { return true; };

    /// Stream operator alias for process()
    virtual void operator<<(IPacket& packet) { process(packet); };
};


typedef PacketProcessor IPacketizer;
typedef PacketProcessor IDepacketizer; /// For 0.8.x compatibility


//
// Packet Adapter Reference
//

/// Provides a reference to a PacketSignal instance.
struct PacketAdapterReference
{
    typedef std::shared_ptr<PacketAdapterReference> Ptr;

    PacketStreamAdapter* ptr;
    ScopedPointer* deleter;
    int order;
    bool syncState;

    PacketAdapterReference(PacketStreamAdapter* ptr = nullptr,
                           ScopedPointer* deleter = nullptr, int order = 0,
                           bool syncState = false)
        : ptr(ptr)
        , deleter(deleter)
        , order(order)
        , syncState(syncState)
    {
    }

    ~PacketAdapterReference()
    {
        if (deleter)
            delete deleter;
    }

    static bool compareOrder(const PacketAdapterReference::Ptr& l,
                             const PacketAdapterReference::Ptr& r)
    {
        return l->order < r->order;
    }
};


typedef std::vector<PacketAdapterReference::Ptr> PacketAdapterVec;


/// Flags which determine how the packet is handled by the PacketStream
enum PacketFlags
{
    NoModify = 0x01, ///< The packet should not be modified by processors.
    Final            ///< The final packet in the stream.
};


//
// Packet Stream State
//


struct PacketStreamState : public State
{
    enum Type
    {
        None = 0,
        Locked,
        Active,
        Paused,
        // Resetting,
        Stopping,
        Stopped,
        Closed,
        Error,
    };

    std::string str(unsigned int id) const
    {
        switch (id) {
            case None:
                return "None";
            case Locked:
                return "Locked";
            case Active:
                return "Active";
            // case Resetting:
            //    return "Resetting";
            case Paused:
                return "Paused";
            case Stopping:
                return "Stopping";
            case Stopped:
                return "Stopped";
            case Closed:
                return "Closed";
            case Error:
                return "Error";
            default:
                assert(false);
        }
        return "undefined";
    }
};


//
// Packet Stream
//


/// This class is used for processing and boradcasting IPackets in a flexible
/// way.
/// A PacketStream consists of one or many PacketSources, one or many
/// PacketProcessors, and one or many delegate receivers.
///
/// This class enables the developer to setup a processor chain in order
/// to perform arbitrary processing on data packets using interchangeable
/// packet adapters, and pump the output to any delegate function,/// or even
/// another PacketStream.
///
/// Note that PacketStream itself inherits from PacketStreamAdapter,/// so a
/// PacketStream be the source of another PacketStream.
///
/// All PacketStream methods are thread-safe, but once the stream is
/// running you will not be able to attach or detach stream adapters.
///
/// In order to synchronize output packets with the application event
/// loop take a look at the SyncPacketQueue class.
/// For lengthy operations you can add an AsyncPacketQueue to the start
/// of the stream to defer processing from the PacketSource thread.
class Base_API PacketStream : public Stateful<PacketStreamState>
{
public:
    typedef std::shared_ptr<PacketStream> Ptr;

    PacketStream(const std::string& name = "");
    virtual ~PacketStream();

    /// Start the stream and synchronized sources.
    virtual void start();

    /// Stop the stream and synchronized sources.
    virtual void stop();

    /// Pause the stream.
    virtual void pause();

    /// Resume the stream.
    virtual void resume();

    /// Close the stream and transition the internal state to Closed.
    virtual void close();

    /// Cleanup all managed stream adapters and reset the stream state.
    virtual void reset();

    /// Returns true when the stream is in the Active state.
    virtual bool active() const;

    /// Returns true when the stream is in the Stopping or Stopped state.
    virtual bool stopped() const;

    /// Returns true when the stream is in the Closed or Error state.
    virtual bool closed() const;

    /// Sets the stream to locked state.
    /// In a locked state no new adapters can be added or removed
    /// from the stream until the stream is stopped.
    virtual bool lock();

    /// Returns true is the stream is currently locked.
    virtual bool locked() const;

    /// Writes data to the stream (nocopy).
    virtual void write(char* data, size_t len);

    /// Writes data to the stream (copied).
    virtual void write(const char* data, size_t len);

    /// Writes an incoming packet onto the stream.
    virtual void write(IPacket&& packet);

    /// Attaches a source packet emitter to the stream.
    /// The source packet adapter can be another PacketStream::emitter.
    virtual void attachSource(PacketSignal& source);

    /// Attaches a source packet emitter to the stream.
    /// If freePointer is true, the pointer will be deleted when the stream is
    /// closed.
    /// If syncState is true and the source is a basic::Stratable, then
    /// the source's start()/stop() methods will be synchronized when
    /// calling startSources()/stopSources().
    virtual void attachSource(PacketStreamAdapter* source, bool freePointer = true, bool syncState = false);

    /// Attaches a source packet emitter to the stream.
    /// This method enables compatibility with shared_ptr managed adapter
    /// instances.
    template <class C>
    void attachSource(std::shared_ptr<C> ptr, bool syncState = false)
    {
        auto source = dynamic_cast<PacketStreamAdapter*>(ptr.get());
        if (!source) {
            assert(0 && "invalid adapter");
            throw std::runtime_error("Cannot attach incompatible packet source.");
        }

        attachSource(std::make_shared<PacketAdapterReference>(
            source, new ScopedSharedPointer<C>(ptr), false, syncState));
    }

    /// Detaches the given source packet signal from the stream.
    virtual bool detachSource(PacketSignal& source);

    /// Detaches the given source packet adapter from the stream.
    /// Note: The pointer will be forgotten about, so if the freePointer
    /// flag set when calling attachSource() will have no effect.
    virtual bool detachSource(PacketStreamAdapter* source);

    /// Attaches a packet processor to the stream.
    /// Order determines the position of the processor in the stream queue.
    /// If freePointer is true, the pointer will be deleted when the stream
    /// closes.
    virtual void attach(PacketProcessor* proc, int order = 0, bool freePointer = true);

    /// Attaches a packet processor to the stream.
    /// This method enables compatibility with shared_ptr managed adapter
    /// instances.
    template <class C>
    void attach(std::shared_ptr<C> ptr, bool syncState = false)
    {
        auto proc = dynamic_cast<PacketProcessor*>(ptr.get());
        if (!proc) {
            assert(0 && "invalid adapter");
            throw std::runtime_error(
                "Cannot attach incompatible packet processor.");
        }

        attach(std::make_shared<PacketAdapterReference>(
            proc, new ScopedSharedPointer<C>(ptr), 0, syncState));
    }

    /// Detaches a packet processor from the stream.
    /// Note: The pointer will be forgotten about, so if the freePointer
    /// flag set when calling attach() will have no effect.
    virtual bool detach(PacketProcessor* proc);

    /// Synchronize stream output packets with the given event loop.
    virtual void synchronizeOutput(uv::Loop* loop);

    /// Set the stream to auto start if inactive (default: false).
    ///
    /// With this flag set the stream will automatically transition to
    /// Active state if the in either the None or Locaked state.
    virtual void autoStart(bool flag);

    /// Set the stream to be closed on error (default: true).
    ///
    /// With this flag set the stream will be automatically transitioned
    /// to Closed state from Error state.
    virtual void closeOnError(bool flag);

    /// Accessors for the unmanaged client data pointer.
    // virtual void setClientData(void* data);
    // virtual void* clientData() const;

    /// Returns the stream error (if any).
    const std::exception_ptr& error();

    /// Returns the name of the packet stream.
    std::string name() const;

    /// Signals to delegates on outgoing packets.
    PacketSignal emitter;

    /// Signals that the PacketStream is in Error state.
    /// If stream output is synchronized then the Error signal will be
    /// sent from the synchronization context, otherwise it will be sent from
    /// the async processor context. See synchronizeOutput()
    Signal<void(PacketStream&, const std::exception_ptr&)> Error;

    /// Signals that the PacketStream is in Close state.
    /// This signal is sent immediately via the close() method,
    /// and as such will be sent from the calling thread context.
    Signal<void(PacketStream&)> Close;

    /// Returns a combined list of all stream sources and processors.
    PacketAdapterVec adapters() const;

    /// Returns a list of all stream sources.
    PacketAdapterVec sources() const;

    /// Returns a list of all stream processors.
    PacketAdapterVec processors() const;

    int numSources() const;
    int numProcessors() const;
    int numAdapters() const;

    template <class AdapterT> AdapterT* getSource(int index = 0)
    {
        int x = 0;
        std::lock_guard<std::mutex> guard(_mutex);
        for (unsigned i = 0; i < _sources.size(); i++) {
            AdapterT* source = dynamic_cast<AdapterT*>(_sources[i]->ptr);
            if (source) {
                if (index == x)
                    return source;
                else
                    x++;
            }
        }
        return nullptr;
    }

    template <class AdapterT> AdapterT* getProcessor(int index = 0)
    {
        int x = 0;
        std::lock_guard<std::mutex> guard(_mutex);
        for (unsigned i = 0; i < _processors.size(); i++) {
            AdapterT* processor = dynamic_cast<AdapterT*>(_processors[i]->ptr);
            if (processor) {
                if (index == x)
                    return processor;
                else
                    x++;
            }
        }
        return nullptr;
    }

    /// Returns the PacketProcessor at the given position.
    PacketProcessor* getProcessor(int order = 0)
    {
        std::lock_guard<std::mutex> guard(_mutex);
        for (unsigned i = 0; i < _processors.size(); i++) {
            PacketProcessor* processor =
                dynamic_cast<PacketProcessor*>(_processors[i]->ptr);
            if (processor && _processors[i]->order == order) {
                return processor;
            }
        }
        return nullptr;
    }

    // Client data pointer
    void* opaque;

protected:
    /// Attach the source and processor delegate chain.
    void setup();

    /// Detach the source and processor delegate chain.
    void teardown();

    void attachSource(PacketAdapterReference::Ptr ref);
    void attach(PacketAdapterReference::Ptr ref);

    /// Start synchronized sources.
    void startSources();

    /// Stop synchronized sources.
    void stopSources();

    /// Process incoming packets.
    virtual void process(IPacket& packet);

    /// Emit the final packet to listeners.
    ///
    /// Synchronized signals such as Close and Error are sent
    /// from this method. See synchronizeOutput()
    void emit(IPacket& packet);

    /// Synchronize queued states with adapters.
    void synchronizeStates();

    /// Override the Stateful::onStateChange method
    virtual void onStateChange(PacketStreamState& state, const PacketStreamState& oldState) override;

    /// Returns true if the given state ID is queued.
    // bool hasQueuedState(PacketStreamState::ID state) const;

    /// Asserts that the stream can be modified, ie is not in the Locked,
    /// Stopping or Active states.
    void assertCanModify();

    /// Handle an internal exception.
    void handleException(std::exception& exc);

    mutable std::mutex _mutex;
    mutable std::mutex _procMutex;
    std::string _name;
    PacketAdapterVec _sources;
    PacketAdapterVec _processors;
    std::deque<PacketStreamState> _states;
    std::exception_ptr _error;
    bool _autoStart;
    bool _closeOnError;
};


typedef std::vector<PacketStream*> PacketStreamVec;
typedef std::vector<PacketStream::Ptr> PacketStreamPtrVec;


} // namespace scy


#endif // SCY_PacketStream_H


/// @\}
