#ifndef UTILS_EXCEPTION_HH
#define UTILS_EXCEPTION_HH

// ============================================================================

/// Declares a new exception class that derives from a base class. The base 
/// class must have a constructor that accepts a std::string and const char*
#define DECLARE_EXCEPTION(cls, parent)          \
    class cls : public parent {                 \
    public:                                     \
        cls (const std::string& a_Message) :    \
            parent(a_Message) {}                \
                                                \
        cls (const char* a_Message) :           \
            parent(a_Message) {}                \
    };                                          \

// ============================================================================

/// Throws an exception with a formatted message as an argument
#define THROW(cls, ...) { \
    throw cls(stringf(__VA_ARGS__)); \
}

// ============================================================================

#endif // UTILS_EXCEPTION_HH
