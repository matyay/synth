#include "worker.hh"

#include <pthread.h>
#include <stdexcept>

// ============================================================================

Worker::~Worker () {
    stop();
}

// ============================================================================

bool Worker::start () {

    // Already running
    if (isAlive()) {
        return false;
    }

    // Reset flag(s)
    m_StopReq.store(false);

    // Run the thread
    m_Thread = std::thread([this]() {this->entry();});
    return true;
}

void Worker::stop () {

    // Not alive
    if (!isAlive()) {
        return;
    }

    // Request stop
    m_StopReq.store(true);

    // Wait for the thread
    m_Thread.join();
}

bool Worker::isAlive () const {
    return m_Thread.joinable();
}

void Worker::setScheduling (int policy, int priority) {
    m_SchedParams.sched_priority = priority;

    if (pthread_setschedparam(m_Thread.native_handle(), policy, &m_SchedParams)) {
        throw std::runtime_error(
            "Failed to set worker's scheduling policy and/or priority!"
        );
    }
}

// ============================================================================

void Worker::entry () {
    int res;

    // Initialize
    res = init();
    if (res) {
        return;
    }

    // The thread loop
    while (!m_StopReq.load()) {
        res = loop();
        if (res) {
            break;
        }
    }
}

