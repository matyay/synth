#ifndef UTILS_BASE_INTERFACE_HH
#define UTILS_BASE_INTERFACE_HH

// ============================================================================

/// Interface ID type
typedef unsigned long iid_t;

/// A macro for creating an interface ID from 4 chars
#define INTERFACE_ID(str) \
    (iid_t)(((str[0]) << 24) | ((str[1]) << 16) | ((str[2]) << 8) | (str[3]))

// ============================================================================

/// Base interface class
class IBaseInterface {
public:

    /// Virtual destructor
    virtual ~IBaseInterface () {};

    /// Queries an interface with the given id. Returns nullptr if not
    /// implemented by the class
    virtual IBaseInterface* queryInterface (iid_t a_Id) = 0;
};

#endif // UTILS_BASE_INTERFACE_HH
