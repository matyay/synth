#ifndef DATA_QUEUE_HH
#define DATA_QUEUE_HH

#include <memory>

#include <cstdint>

#include <pthread.h>
#include <semaphore.h>

// ============================================================================

class DataQueue
{
public:

    /// Constructs a queue with the given size
     DataQueue (size_t a_Size);
    ~DataQueue ();

    DataQueue (const DataQueue&  ref) = delete;
    DataQueue (const DataQueue&& ref) = delete;
    void operator = (const DataQueue& ref) = delete;

    /// Returns the queue size
    size_t getSize      () const;
    /// Returns the occupied size
    size_t getOccupancy ();
    /// Returns the remaining size
    size_t getRemaining ();

    /// Clears the queue
    void clear ();
    
    /// Puts  data block into the queue
    int put  (const void* a_Ptr, size_t a_Size, int a_Timeout = -1);
    /// Pops data block from the queue
    int pop  (void* a_Ptr, size_t a_Size, int a_Timeout = -1);
    /// Peeks  data block from the queue without removing it
    int peek (void* a_Ptr, size_t a_Size, int a_Timeout = -1);

private:

    /// Size
    size_t m_Size;
    /// Occupied size
    size_t m_Occupancy = 0;
    
    /// The ring buffer
    std::unique_ptr<uint8_t> m_Buffer;
    /// Ring buffer read pointer
    size_t m_ReadPtr  = 0;
    /// Ring buffer write pointer
    size_t m_WritePtr = 0;

    /// Minimal data size that we are waiting for to be put
    size_t m_ReadRequest  = 0;
    /// Minimal amount of data that we are waiting to be pop'ed
    size_t m_WriteRequest = 0;

    /// Access mutex
    pthread_mutex_t m_Mutex;
    /// Put semaphore
    sem_t m_SemPut;
    /// Pop/peek semaphore
    sem_t m_SemPop;

    
    /// Waits for a semaphore with relative timeout
    static int semWait (sem_t& a_Sem, int a_Timeout);
};

// ============================================================================

#endif // DATA_QUEUE_HH
