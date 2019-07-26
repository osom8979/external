///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier: LGPL-2.1+
//
/// @addtogroup sched
/// @{


#include "scy/sched/task.h"
#include "scy/datetime.h"
#include "scy/sched/scheduler.h"


using namespace std;


namespace scy {
namespace sched {


Task::Task(const std::string& type, const std::string& name)
    : _type(type)
    , _name(name)
    , _scheduler(nullptr)
    , _trigger(nullptr)
{
    LTrace("Create")
}


Task::Task(sched::Scheduler& scheduler, const std::string& type,
           const std::string& name)
    : _type(type)
    , _name(name)
    , _scheduler(&scheduler)
    , _trigger(nullptr)
{
    LTrace("Create")
}


Task::~Task()
{
    LTrace("Destroy")
}


/*
void Task::start()
{
    trigger(); // throw if trigger is nullptr
    scy::Task::start();
}
*/


void Task::serialize(json::value& root)
{
    LTrace("Serializing")

    std::lock_guard<std::mutex> guard(_mutex);

    root["id"] = _id;
    root["type"] = _type;
    root["name"] = _name;
}


void Task::deserialize(json::value& root)
{
    LTrace("Deserializing")

    std::lock_guard<std::mutex> guard(_mutex);

    json::assertMember(root, "id");
    json::assertMember(root, "type");
    json::assertMember(root, "name");

    _id = root["id"].get<uint32_t>();
    _type = root["type"].get<std::string>();
    _name = root["name"].get<std::string>();
}


bool Task::beforeRun()
{
    std::lock_guard<std::mutex> guard(_mutex);
    return _trigger && _trigger->timeout() && !_destroyed && !cancelled();
}


bool Task::afterRun()
{
    std::lock_guard<std::mutex> guard(_mutex);
    DateTime now;
    _trigger->update();
    _trigger->timesRun++;
    _trigger->lastRunAt = now;
    return !_trigger->expired();
}


void Task::setTrigger(sched::Trigger* trigger)
{
    std::lock_guard<std::mutex> guard(_mutex);
    if (_trigger)
        delete _trigger;
    _trigger = trigger;
}


string Task::name() const
{
    std::lock_guard<std::mutex> guard(_mutex);
    return _name;
}


string Task::type() const
{
    std::lock_guard<std::mutex> guard(_mutex);
    return _type;
}


std::int64_t Task::remaining() const
{
    std::lock_guard<std::mutex> guard(_mutex);
    if (!_trigger)
        throw std::runtime_error("Tasks must be have a Trigger instance.");
    return _trigger->remaining();
}


sched::Trigger& Task::trigger()
{
    std::lock_guard<std::mutex> guard(_mutex);
    if (!_trigger)
        throw std::runtime_error("Tasks must have a Trigger instance.");
    return *_trigger;
}


sched::Scheduler& Task::scheduler()
{
    std::lock_guard<std::mutex> guard(_mutex);
    if (!_scheduler)
        throw std::runtime_error(
            "Tasks must be started with a sched::Scheduler instance.");
    return *_scheduler;
}


} // namespace sched
} // namespace scy


/// @\}
