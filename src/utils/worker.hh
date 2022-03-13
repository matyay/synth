#ifndef WORKER_HH
#define WORKER_HH

#include <thread>
#include <atomic>

// ============================================================================

class Worker {
public:

    Worker () = default;
    virtual ~Worker ();

    /// Start
    virtual bool start ();
    /// Stop
    virtual void stop  ();

    /// Returns true if the thread is alive
    bool isAlive () const;

    /// Sets scheduling (POSIX)
    void setScheduling (int policy, int priority);

protected:

    /// Initialization function
    virtual int  init  () {return 0;}
    /// The work loop function
    virtual int  loop  () = 0;

    /// Thread entry point
    void         entry ();

    /// The worker thread
    std::thread         m_Thread;
    /// Stop request
    std::atomic_bool    m_StopReq;

    /// POSIX scheduling parameters
    sched_param         m_SchedParams;
};

#endif
