///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier: LGPL-2.1+
//
/// @addtogroup base
/// @{


#include "scy/packetstream.h"
#include "scy/memory.h"
#include "scy/packetqueue.h"


using std::endl;


namespace scy {


PacketStream::PacketStream(const std::string& name)
    : opaque(nullptr)
    , _name(name)
    , _autoStart(false)
    , _closeOnError(true)
{
}


PacketStream::~PacketStream()
{
    close();

    // Delete managed adapters
    reset();

    // The event machine should always be complete
    assert(stateEquals(PacketStreamState::None) ||
           stateEquals(PacketStreamState::Closed) ||
           stateEquals(PacketStreamState::Error));

    // Make sure all adapters have been cleaned up
    assert(_sources.empty());
    assert(_processors.empty());
}


void PacketStream::start()
{
    LTrace("Start")

    if (stateEquals(PacketStreamState::Active)) {
        LTrace("Start: Already active")
        // assert(0);
        return;
    }

    // Setup the delegate chain
    setup();

    // Set state to Active
    setState(this, PacketStreamState::Active);

    // Lock the processor mutex to synchronize multi source streams
    std::lock_guard<std::mutex> guard(_procMutex);

    // Start synchronized sources
    startSources();
}


void PacketStream::stop()
{
    LTrace("Stop")

    if (stateEquals(PacketStreamState::Stopped) ||
        stateEquals(PacketStreamState::Stopping) ||
        stateEquals(PacketStreamState::Closed)) {
        LTrace("Stop: Already stopped")
        // assert(0);
        return;
    }

    setState(this, PacketStreamState::Stopping);
    setState(this, PacketStreamState::Stopped);

    // Lock the processor mutex to synchronize multi source streams
    std::lock_guard<std::mutex> guard(_procMutex);

    // Stop synchronized sources
    stopSources();
}


void PacketStream::pause()
{
    LTrace("Pause")
    setState(this, PacketStreamState::Paused);
}


void PacketStream::resume()
{
    LTrace("Resume")
    if (!stateEquals(PacketStreamState::Paused)) {
        LTrace("Resume: Not paused")
        return;
    }

    setState(this, PacketStreamState::Active);
}


void PacketStream::reset()
{
    LTrace("Reset")
    assert(stateEquals(PacketStreamState::None) ||
           stateEquals(PacketStreamState::Closed));

    std::lock_guard<std::mutex> guard(_mutex);
    _sources.clear(); // not detaching here
    _processors.clear(); // not detaching here
}


void PacketStream::close()
{
    if (stateEquals(PacketStreamState::None) ||
        stateEquals(PacketStreamState::Closed)) {
        // LTrace("Already closed")
        // assert(0);
        return;
    }

    // Stop the stream gracefully (if running)
    if (!stateEquals(PacketStreamState::Stopped) &&
        !stateEquals(PacketStreamState::Stopping))
        stop();

    LTrace("Closing")

    // Queue the Closed state
    setState(this, PacketStreamState::Closed);

    {
        // Lock the processor mutex to synchronize multi source streams
        std::lock_guard<std::mutex> guard(_procMutex);

        // Teardown the adapter delegate chain
        teardown();

        // Synchronize any pending states
        // This should be safe since the adapters won't be receiving
        // any more incoming packets after teardown.
        // This call is essential when using the event loop otherwise
        // failing to close some handles could result in deadlock.
        // See SyncQueue::cancel()
        synchronizeStates();

        // Clear and delete managed adapters
        // Note: Can't call this because if closeOnCleanup is true
        // we may be inside a queue loop which will be destroyed by
        // the call to reset()
        // reset();
    }

    // Send the Closed signal
    Close.emit(*this);
}


void PacketStream::write(char* data, size_t len)
{
    write(RawPacket(data, len));
}


void PacketStream::write(const char* data, size_t len)
{
    write(RawPacket(data, len));
}

void PacketStream::write(IPacket&& packet)
{
    process(packet);
}


bool PacketStream::locked() const
{
    return stateEquals(PacketStreamState::Locked);
}


bool PacketStream::lock()
{
    if (!stateEquals(PacketStreamState::None))
        return false;

    setState(this, PacketStreamState::Locked);
    return true;
}


bool PacketStream::active() const
{
    return stateEquals(PacketStreamState::Active);
}


bool PacketStream::closed() const
{
    return stateEquals(PacketStreamState::Closed) ||
           stateEquals(PacketStreamState::Error);
}


bool PacketStream::stopped() const
{
    return stateEquals(PacketStreamState::Stopping) ||
           stateEquals(PacketStreamState::Stopped);
}


void PacketStream::autoStart(bool flag)
{
    assertCanModify();
    std::lock_guard<std::mutex> guard(_mutex);
    _autoStart = flag;
}


void PacketStream::closeOnError(bool flag)
{
    assertCanModify();
    std::lock_guard<std::mutex> guard(_mutex);
    _closeOnError = flag;
}


std::string PacketStream::name() const
{
    std::lock_guard<std::mutex> guard(_mutex);
    return _name;
}


//
// Packet Stream Base
//


void PacketStream::synchronizeStates()
{
    // Process queued internal states first
    while (!_states.empty()) {
        PacketStreamState state;
        {
            std::lock_guard<std::mutex> guard(_mutex);
            state.set(_states.front().id());
            _states.pop_front();
        }

        LTrace("Set queued state: ", state)

        // Send the stream state to packet adapters.
        // This is done inside the processor thread context so
        // packet adapters do not need to consider thread safety.
        auto adapters = this->adapters();
        for (auto& ref : adapters) {
            reinterpret_cast<PacketStreamAdapter*>(ref->ptr)->onStreamStateChange(state);
        }
    }
}


void PacketStream::process(IPacket& packet)
{
    STrace << "Processing packet: " << state() << ": "
                 << packet.className() << endl;
    // assert(Thread::currentID() == _runner->tid());

    try {

        // Auto start the stream if required and currently inactive
        if (_autoStart && stateEquals(PacketStreamState::None))
            start();

        // Process the packet if the stream is active
        PacketProcessor* firstProc = nullptr;
        if (stateEquals(PacketStreamState::Active) &&
            !packet.flags.has(PacketFlags::NoModify)) {
            {
                std::lock_guard<std::mutex> guard(_mutex);
                firstProc = !_processors.empty() ?
                    reinterpret_cast<PacketProcessor*>(_processors[0]->ptr) : nullptr;
            }
            if (firstProc) {

                // Lock the processor mutex to synchronize multi source streams
                std::lock_guard<std::mutex> guard(_procMutex);

                // Sync queued states
                synchronizeStates();

                // Send the packet to the first processor in the chain
                if (stateEquals(PacketStreamState::Active)) {
                    // STrace << "Starting process chain: "
                    //     << firstProc << ": " << packet.className() << endl;
                    // assert(stateEquals(PacketStreamState::Active));
                    if (firstProc->accepts(&packet))
                        firstProc->process(packet);
                    else
                        firstProc->emit(packet);

                    // STrace << "After process chain: "
                    //     << firstProc << ": " << packet.className() << endl;
                    // If all went well the packet was processed and emitted...
                }

                // Proxy packets which are rejected by the first processor
                else {
                    SWarn << "Source packet rejected: " << firstProc
                                << ": " << packet.className() << endl;
                    firstProc = nullptr;
                }
            }
        }

        // Otherwise just proxy and emit the packet
        // TODO: Should we pass the packet to the PacketSyncQueue if
        // synchronizeOutput was used?
        if (!firstProc) {
            STrace << "Proxying packet: " << state() << ": "
                         << packet.className() << endl;
            emit(packet);
        }

        // Stop the packet stream on the final packet
        if (stateEquals(PacketStreamState::Active) &&
            packet.flags.has(PacketFlags::Final)) {
            this->stop();
        }
    }

    // Catch any exceptions thrown within the processor
    catch (std::exception& exc) {
        handleException(exc);
    }

    // STrace << "End process chain: "
    //     << state() << ": " << packet.className() << endl;
}


void PacketStream::emit(IPacket& packet)
{
    LTrace("Emit: ", packet.size())

    // Ensure the stream is still running
    if (!stateEquals(PacketStreamState::Active)) {
        LTrace("Dropping packet on inactive stream: ", state())
        return;
    }

    try {
        emitter.emit(packet);
    } catch (std::exception& exc) {
        handleException(exc);
    }
}


void PacketStream::setup()
{
    try {
        std::lock_guard<std::mutex> guard(_mutex);

        // Setup the processor chain
        PacketProcessor* lastProc = nullptr;
        PacketProcessor* thisProc = nullptr;
        for (auto& proc : _processors) {
            thisProc = reinterpret_cast<PacketProcessor*>(proc->ptr);
            if (lastProc) {
                lastProc->getEmitter() += slot(thisProc, &PacketProcessor::process);
            }
            lastProc = thisProc;
        }

        // The last processor will emit the packet to the application
        if (lastProc)
            lastProc->getEmitter() += slot(this, &PacketStream::emit);

        // Attach source emitters to the PacketStream::process method
        for (auto& source : _sources) {
            source->ptr->getEmitter() += slot(this, &PacketStream::process);
        }
    } catch (std::exception& exc) {
        LError("Cannot start stream: ", exc.what())
        setState(this, PacketStreamState::Error); //, exc.what()
        throw exc;
    }
}


void PacketStream::teardown()
{
    LTrace("Teardown")
    std::lock_guard<std::mutex> guard(_mutex);

    // Detach the processor chain first
    PacketProcessor* lastProc = nullptr;
    PacketProcessor* thisProc = nullptr;
    for (auto& proc : _processors) {
        thisProc = reinterpret_cast<PacketProcessor*>(proc->ptr);
        if (lastProc)
            lastProc->getEmitter() -= slot(thisProc, &PacketProcessor::process);
        lastProc = thisProc;
    }
    if (lastProc)
        lastProc->getEmitter() -= slot(this, &PacketStream::emit);

    // Detach sources
    for (auto& source : _sources) {
        source->ptr->getEmitter() -= slot(this, &PacketStream::process);
    }
}


void PacketStream::attachSource(PacketStreamAdapter* source, bool freePointer, bool syncState)
{
    // LTrace("Attach source: ", source)
    attachSource(std::make_shared<PacketAdapterReference>(
        source, freePointer ? new ScopedRawPointer<PacketStreamAdapter>(source) : nullptr, 0, syncState));
}


void PacketStream::attachSource(PacketAdapterReference::Ptr ref)
{
    assertCanModify();

    std::lock_guard<std::mutex> guard(_mutex);
    _sources.push_back(ref);
    std::sort(_sources.begin(), _sources.end(), PacketAdapterReference::compareOrder);
}


void PacketStream::attachSource(PacketSignal& source)
{
    // LTrace("Attach source signal: ", &source)
    assertCanModify();

    // TODO: unique_ptr for exception safe pointer creation so we
    // don't need to do state checks here as well as
    // `attachSource(PacketStreamAdapter*)`
    attachSource(new PacketStreamAdapter(source), true, false);
}


bool PacketStream::detachSource(PacketStreamAdapter* source)
{
    // LTrace("Detach source adapter: ", source)
    assertCanModify();

    std::lock_guard<std::mutex> guard(_mutex);
    for (auto it = _sources.begin(); it != _sources.end(); ++it) {
        if ((*it)->ptr == source) {
            (*it)->ptr->getEmitter() -= slot(this, &PacketStream::process);
            _sources.erase(it);
            return true;
        }
    }
    return false;
}


bool PacketStream::detachSource(PacketSignal& source)
{
    // LTrace("Detach source signal: ", &source)
    assertCanModify();

    std::lock_guard<std::mutex> guard(_mutex);
    for (auto it = _sources.begin(); it != _sources.end(); ++it) {
        if (&(*it)->ptr->getEmitter() == &source) {
            (*it)->ptr->getEmitter() -= slot(this, &PacketStream::process);
            return true;
        }
    }
    return false;
}


void PacketStream::attach(PacketProcessor* proc, int order, bool freePointer)
{
    // LTrace("Attach processor: ", proc)
    assert(order >= -1 && order <= 101);
    assertCanModify();

    std::lock_guard<std::mutex> guard(_mutex);
    _processors.push_back(std::make_shared<PacketAdapterReference>(proc,
        freePointer ? new ScopedRawPointer<PacketStreamAdapter>(proc) : nullptr,
        order == -1 ? int(_processors.size()) : order));

    sort(_processors.begin(), _processors.end(), PacketAdapterReference::compareOrder);
}


bool PacketStream::detach(PacketProcessor* proc)
{
    // LTrace("Detach processor: ", proc)
    assertCanModify();

    std::lock_guard<std::mutex> guard(_mutex);
    for (auto it = _processors.begin(); it != _processors.end(); ++it) {
        if ((*it)->ptr == proc) {
            LTrace("Detached processor: ", proc)
            _processors.erase(it);
            return true;
        }
    }
    return false;
}


void PacketStream::attach(PacketAdapterReference::Ptr ref)
{
    assertCanModify();

    std::lock_guard<std::mutex> guard(_mutex);
    _processors.push_back(ref);
    std::sort(_processors.begin(), _processors.end(), PacketAdapterReference::compareOrder);
}


void PacketStream::startSources()
{

    auto sources = this->sources();
    for (auto& source : sources) {
        if (source->syncState) {
            auto startable = dynamic_cast<basic::Startable*>(source->ptr);
            if (startable) {
                LTrace("Start source: ", startable)
                startable->start();
            } else
                assert(0 && "unknown synchronizable");
#if 0
            auto runnable = dynamic_cast<basic::Runnable*>(source);
            if (runnable) {
                LTrace("Starting runnable: ", source)
                runnable->run();
            }
#endif
        }
    }
}


void PacketStream::stopSources()
{
    auto sources = this->sources();
    for (auto& source : sources) {
        if (source->syncState) {
            auto startable = dynamic_cast<basic::Startable*>(source->ptr);
            if (startable) {
                LTrace("Stop source: ", startable)
                startable->stop();
            } else
                assert(0 && "unknown synchronizable");
#if 0
            auto runnable = dynamic_cast<basic::Runnable*>(source);
            if (runnable) {
                LTrace("Stop runnable: ", source)
                runnable->cancel();
            }
#endif
        }
    }
}


void PacketStream::handleException(std::exception& exc)
{
    LError("Error: ", exc.what())

    // Set the stream Error state. No need for queueState
    // as we are currently inside the processor context.
    setState(this, PacketStreamState::Error); //, exc.what()

    // Capture the exception so it can be rethrown elsewhere.
    // The Error signal will be sent on next call to emit()
    _error = std::current_exception();
    Error.emit(*this, _error);

    //_syncError = true;
    if (_closeOnError) {
        LTrace("Close on error")
        this->close();
    }
}


void PacketStream::assertCanModify()
{
    if (stateEquals(PacketStreamState::Locked) ||
        stateEquals(PacketStreamState::Stopping) ||
        stateEquals(PacketStreamState::Active)) {
        LError("Cannot modify an ", state(), " packet stream")
        assert(0 && "cannot modify active packet stream");
        throw std::runtime_error("Cannot modify an active packet stream.");
    }
}


void PacketStream::synchronizeOutput(uv::Loop* loop)
{
    assertCanModify();

    // Add a SyncPacketQueue as the final processor so output
    // packets will be synchronized when they hit the emit() method
    attach(new SyncPacketQueue<>(loop), 101, true);
}


void PacketStream::onStateChange(PacketStreamState& state, const PacketStreamState& oldState)
{
    LTrace("On state change: ", oldState, " => ", state)

    // Queue state for passing to adapters
    std::lock_guard<std::mutex> guard(_mutex);
    _states.push_back(state);
}


const std::exception_ptr& PacketStream::error()
{
    std::lock_guard<std::mutex> guard(_mutex);
    return _error;
}


int PacketStream::numSources() const
{
    std::lock_guard<std::mutex> guard(_mutex);
    return int(_sources.size());
}


int PacketStream::numProcessors() const
{
    std::lock_guard<std::mutex> guard(_mutex);
    return int(_processors.size());
}


int PacketStream::numAdapters() const
{
    std::lock_guard<std::mutex> guard(_mutex);
    return int(_sources.size() + _processors.size());
}


PacketAdapterVec PacketStream::adapters() const
{
    std::lock_guard<std::mutex> guard(_mutex);
    PacketAdapterVec res(_sources);
    res.insert(res.end(), _processors.begin(), _processors.end());
    return res;
}


PacketAdapterVec PacketStream::sources() const
{
    std::lock_guard<std::mutex> guard(_mutex);
    return _sources;
}


PacketAdapterVec PacketStream::processors() const
{
    std::lock_guard<std::mutex> guard(_mutex);
    return _processors;
}


//
// Packet Stream Adapter
//


PacketStreamAdapter::PacketStreamAdapter(PacketSignal& emitter)
    : _emitter(emitter)
{
}


void PacketStreamAdapter::emit(char* data, size_t len, unsigned flags)
{
    RawPacket p(data, len, flags);
    emit(p);
}


void PacketStreamAdapter::emit(const char* data, size_t len,
                               unsigned flags)
{
    RawPacket p(data, len, flags);
    emit(p);
}


void PacketStreamAdapter::emit(const std::string& str, unsigned flags)
{
    RawPacket p(str.c_str(), str.length(), flags);
    emit(p);
}


void PacketStreamAdapter::emit(unsigned flags)
{
    FlagPacket p(flags);
    emit(p);
}


void PacketStreamAdapter::emit(IPacket& packet)
{
    getEmitter().emit(packet);
}


PacketSignal& PacketStreamAdapter::getEmitter()
{
    return _emitter;
}


//
// Helper Functions
//


// bool PacketStream::hasQueuedState(PacketStreamState::ID state) const
// {
//     std::lock_guard<std::mutex> guard(_mutex);
//     for (auto const& st : _states) {
//         if (st.id() == state)
//             return true;
//     }
//     return false;
// }


// bool waitForStateSync(PacketStream* stream, PacketStreamState::ID state)
// {
//     int times = 0;
//     LTrace(stream)("Wait for sync state: ", state)
//     while (!stream->stateEquals(state) || stream->hasQueuedState(state)) {
//         STrace(stream) << "Wait for sync state: " << state << ": " << times
//                      << endl;
//         scy::sleep(5);
//         if (times++ > 1000) {
//             assert(0 && "deadlock; calling inside stream scope?"); // 5 secs
//         }
//     }
//     LTrace(stream), "Wait for sync state: "(state, ": OK")
//     return true;
// }


} // namespace scy


/// @\}
