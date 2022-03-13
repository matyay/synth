#ifndef AUDIO_BUFFER_HH
#define AUDIO_BUFFER_HH

#include "exception.hh"

#include <memory>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace Audio {

// ============================================================================

/// A generic multi-channel audio buffer. Supports basic arithmetic operations.
template <typename T>
class Buffer {
public:

    /// Constructor
    Buffer (size_t a_Size = 0, size_t a_Channels = 1) : 
        m_Size      (0),
        m_Channels  (1)
    {
        assert(m_Channels >= 1);
        create(a_Size, a_Channels);
    }

    /// Copy constructor
    Buffer (const Buffer& ref) :
        m_Size      (ref.m_Size),
        m_Channels  (ref.m_Channels),
        m_Data      (ref.m_Data)
    {}

    // ........................................................................

    /// Creates the buffer
    void create (size_t a_Size, size_t a_Channels = 1) {
        assert(m_Channels >= 1);

        if (a_Size != m_Size || a_Channels != m_Channels) {
            m_Size     = a_Size;
            m_Channels = a_Channels;

            if (m_Size != 0) {
                size_t count = a_Size * a_Channels;
                m_Data.reset(new T[count]);
            }
            else {
                m_Data.reset();
            }
        }
    }

    /// Releases the buffer
    void release () {
        m_Size     = 0;
        m_Data.reset();
    }

    // ........................................................................

    /// Assignment
    void operator = (const Buffer<T>& ref) {
        m_Size     = ref.m_Size;
        m_Channels = ref.m_Channels;
        m_Data     = ref.m_Data;
    }

    /// Creates a copy
    Buffer<T> copy () const {
        Buffer<T> buffer (m_Size, m_Channels);
        copyTo(buffer);

        return buffer;
    }

    /// Copies data to another buffer
    void copyTo (Buffer<T>& ref) const {
        check(ref);

        if (m_Size) {
            size_t count = m_Size * m_Channels;       
            memcpy(ref.m_Data.get(), m_Data.get(), sizeof(T) * count);
        }
    }

    // ........................................................................

    /// Returns size
    inline size_t getSize () const {
        return m_Size;
    }

    /// Returns channel count
    inline size_t getChannels () const {
        return m_Channels;
    }

    /// Returns a pointer to the channel data
    inline T* data (size_t a_Channel = 0) {
        T* ptr = m_Data.get();
        return ptr + (m_Size * a_Channel);
    }

    /// Returns a constant pointer to the channel data
    inline const T* data (size_t a_Channel = 0) const {
        const T* ptr = m_Data.get();
        return ptr + (m_Size * a_Channel);
    }

    /// Checks compatibility of two buffers
    inline bool isCompatible (const Buffer<T>& ref) const {
        return (m_Channels == ref.m_Channels) && (m_Size == ref.m_Size);
    }

    // ........................................................................

    /// Clears the buffer
    void clear () {
        if (m_Size) {
            size_t count = m_Size * m_Channels;
            memset(m_Data.get(), 0, count * sizeof(T));
        }
    }

    /// Fills the buffer with the given value
    void fill (T val) {
        size_t count = m_Size * m_Channels;
        T*     ptr   = m_Data.get();
        size_t i;

        for (i=0; i<(count-4); i=i+4) {
            *ptr++ = val;
            *ptr++ = val;
            *ptr++ = val;
            *ptr++ = val;
        }

        for (; i<count; ++i) {
            *ptr++ = val;
        }
    }

    // ........................................................................

    Buffer<T> operator *= (T k) {
        size_t count = m_Size * m_Channels;
        T*     ptr   = m_Data.get();
        size_t i;

        for (i=0; i<(count-4); i=i+4) {
            *ptr++ *= k;
            *ptr++ *= k;
            *ptr++ *= k;
            *ptr++ *= k;
        }

        for (; i<count; ++i) {
            *ptr++ *= k;
        }
    }

    Buffer<T> operator += (const Buffer<T>& ref) {
        check(ref);

        size_t count = m_Size * m_Channels;
        T*     dst   = m_Data.get();
        T*     src   = ref.m_Data.get();
        size_t i;

        for (i=0; i<(count-4); i=i+4) {
            *dst++ += *src++;
            *dst++ += *src++;
            *dst++ += *src++;
            *dst++ += *src++;
        }

        for (; i<count; ++i) {
            *dst++ += *src++;
        }

        return *this;
    }

    Buffer<T> operator *= (const Buffer<T>& ref) {
        check(ref);

        size_t count = m_Size * m_Channels;
        T*     dst   = m_Data.get();
        T*     src   = ref.m_Data.get();
        size_t i;

        for (i=0; i<(count-4); i=i+4) {
            *dst++ *= *src++;
            *dst++ *= *src++;
            *dst++ *= *src++;
            *dst++ *= *src++;
        }

        for (; i<count; ++i) {
            *dst++ *= *src++;
        }

        return *this;
    }

    // ........................................................................

protected:

    /// Throws an exception if buffers are not compatible
    inline void check (const Buffer<T>& other) const {

        if (m_Channels != other.m_Channels) {
            throw ProcessingError(
                "Audio buffers have different channel counts!"
            );
        }

        if (m_Size != other.m_Size) {
            throw ProcessingError(
                "Audio buffers have different sizes!"
            );
        }
    }

    /// Size (in frames)
    size_t  m_Size;
    /// Number of channels
    size_t  m_Channels;

    /// Data
    std::shared_ptr<T> m_Data;
};

// ============================================================================

}; // Audio

#endif // AUDIO_BUFFER_HH

