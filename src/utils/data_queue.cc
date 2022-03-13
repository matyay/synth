#include "data_queue.hh"

#include <cstring>

// ============================================================================

DataQueue::DataQueue (size_t a_Size) :
    m_Size (a_Size)
{
    // Allocate buffer
    m_Buffer.reset(new uint8_t[a_Size]);
    
    // Initialize synchronization objects
    pthread_mutex_init(&m_Mutex, NULL);
    
    sem_init(&m_SemPut, 0, 1);
    sem_init(&m_SemPop, 0, 1);    
}

DataQueue::~DataQueue() {

    pthread_mutex_destroy(&m_Mutex);
    
    sem_destroy(&m_SemPut);    
    sem_destroy(&m_SemPop);    
}

// ============================================================================

size_t DataQueue::getSize () const {
    return m_Size;
}

size_t DataQueue::getOccupancy () {
    size_t  res;

    pthread_mutex_lock(&m_Mutex);
    res = m_Occupancy;
    pthread_mutex_unlock(&m_Mutex);

    return res;
}

size_t DataQueue::getRemaining () {
    size_t  res;

    pthread_mutex_lock(&m_Mutex);    
    res = m_Size - m_Occupancy;
    pthread_mutex_unlock(&m_Mutex);

    return res;
}

// ============================================================================

void DataQueue::clear () {

    pthread_mutex_lock(&m_Mutex);

    // Reset
    m_Occupancy = 0;
    m_ReadPtr   = 0;
    m_WritePtr  = 0;    
    
    m_ReadRequest  = 0;
    m_WriteRequest = 0;
    
    sem_init(&m_SemPut, 0, 1);
    sem_init(&m_SemPop, 0, 1);    
    
    pthread_mutex_unlock(&m_Mutex);
}

// ============================================================================

int DataQueue::semWait (sem_t& a_Sem, int a_Timeout) {

    // No waiting, just poll
    if (a_Timeout == 0) {
        if (sem_trywait(&a_Sem) < 0) {
            if (errno == EAGAIN) {
                return 1;
            }
            else {
                return -1;
            }
        }

        return 0;
    }

    // Inifinte wait
    else if (a_Timeout < 0) {
        if (sem_wait(&a_Sem) < 0) {
            return -1;
        }

        return 0;
    }

    // Wait with timeout
    else {

        // Get current time
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);

        // Set the deadline
        uint64_t deadline = ts.tv_sec * (uint64_t)1000000000L + ts.tv_nsec;
        deadline += a_Timeout * 1000000L;

        ts.tv_sec  = deadline / 1000000000L;
        ts.tv_nsec = deadline % 1000000000L;

        // Wait
        if (sem_timedwait(&a_Sem, &ts) < 0) {
            if (errno == ETIMEDOUT) {
                return 1;
            }
            else {
                return -1;
            }
        }

        return 0;
    }
}

// ============================================================================

// Put
int DataQueue::put (const void* a_Ptr, size_t a_Size, int a_Timeout) {

    // Error check
    if (a_Size > m_Size) {
        return -1;
    }

    pthread_mutex_lock(&m_Mutex);
    
    // Write request pending?
    if (m_WriteRequest) {
        pthread_mutex_unlock(&m_Mutex);
        return -1;
    }
    
    // No free space, set lock and wait
    if ((m_Size - m_Occupancy) < a_Size) {
        m_WriteRequest = a_Size;

        pthread_mutex_unlock(&m_Mutex);
        int res = semWait(m_SemPut, a_Timeout);
        if (res) {
            return res;
        }

        pthread_mutex_lock(&m_Mutex);
    }

    // Put
    size_t   size = a_Size;
    uint8_t* ptr  = (uint8_t*)a_Ptr;
    uint8_t* buf  = (uint8_t*)m_Buffer.get();
    
    while (size) {

        size_t chunkSize = m_Size - m_WritePtr;
        if(chunkSize > size) chunkSize = size;

        memcpy(&buf[m_WritePtr], ptr, chunkSize);

        m_WritePtr    = (m_WritePtr + chunkSize) % m_Size;
        m_Occupancy  += chunkSize;
        ptr          += chunkSize;
        size         -= chunkSize;
    }

    // Remove read lock
    if (m_ReadRequest && m_Occupancy >= m_ReadRequest) {
        m_ReadRequest = 0;
        sem_post(&m_SemPop);
    }

    pthread_mutex_unlock(&m_Mutex);    
    return 0;
}

// Pop
int DataQueue::pop (void* a_Ptr, size_t a_Size, int a_Timeout) {

    // Error check
    if (a_Size > m_Size) {
        return -1;
    }

    pthread_mutex_lock(&m_Mutex);

    // Read request pending?
    if (m_ReadRequest) {
        pthread_mutex_unlock(&m_Mutex);
        return -1;
    }
    
    // Not enough data, set lock and wait
    if (m_Occupancy < a_Size) {
        m_ReadRequest = a_Size;

        pthread_mutex_unlock(&m_Mutex);
        int res = semWait(m_SemPop, a_Timeout);
        if (res) {
            return res;
        }

        pthread_mutex_lock(&m_Mutex);
    }

    // Pop
    if (!a_Ptr) {   // Just move pointer
        m_ReadPtr    = (m_ReadPtr + a_Size) % m_Size;
        m_Occupancy -= a_Size;
    }
    else { // Read actual data
        size_t    size  = a_Size;
        uint8_t*  ptr   = (uint8_t*)a_Ptr;
        uint8_t*  buf   = (uint8_t*)m_Buffer.get();

        while (size) {

            size_t chunkSize = m_Size - m_ReadPtr;
            if(chunkSize > size) chunkSize = size;

            memcpy(ptr, &buf[m_ReadPtr], chunkSize);

            m_ReadPtr    = (m_ReadPtr + chunkSize) % m_Size;
            m_Occupancy -= chunkSize;
            ptr         += chunkSize;
            size        -= chunkSize;
        }
    }
    
    // Remove write lock
    if (m_WriteRequest && (m_Size - m_Occupancy) >= m_WriteRequest ) {
        m_WriteRequest = 0;
        sem_post(&m_SemPut);
    }

    pthread_mutex_unlock(&m_Mutex);    
    return 0;
}

int DataQueue::peek (void* a_Ptr, size_t a_Size, int a_Timeout) {
    (void)a_Ptr;
    (void)a_Size;
    (void)a_Timeout;

    return -1;
}

