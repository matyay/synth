#ifndef DICT_HH
#define DICT_HH

#include <unordered_map>

// ============================================================================

/// A dictionary
template <typename K, typename V>
class Dict
{
public:

    /// Default constructor
    Dict () = default;
    /// Initializing constructor
    Dict (const std::unordered_map<K, V>& a_Content) :
        m_Content(a_Content) {}
    /// Copy constructor
    Dict (const Dict<K, V>& ref) :
        m_Content(ref.m_Content) {}
    /// Move constructor
    Dict (const Dict<K, V>&& ref) :
        m_Content(ref.m_Content) {}

    /// Assignment
    void operator = (const Dict<K, V>& ref) {
        m_Content = ref.m_Content;
    }

    // ....................................................

    /// Returns true when empty
    bool   empty () const;
    /// Returns the element count
    size_t size  () const;
    /// Clears content
    void   clear ();

    /// Returns true if the key is defined.
    bool has (const K& a_Key) const;

    /// Returns the key
    V&       get (const K& a_Key);
    /// Returns the key
    const V& get (const K& a_Key) const;
    /// Returns the key if present or the given default value if not.
    V&       get (const K& a_Key, V& a_Default);
    /// Returns the key if present or the given default value if not.
    const V& get (const K& a_Key, const V& a_Default) const;

    /// Adds / sets the key to the given value
    void set (const K& a_Key, const V& a_Value);

    /// Deletes a key if present
    void del (const K& a_Key); 
    /// Deletes a key pointed by an iterator. Returns its next value
    typename std::unordered_map<K, V>::iterator del (
        const typename std::unordered_map<K, V>::iterator pos
    );

    /// Updates with data from another Dict
    void update (const Dict<K, V>& a_Other);

    // ....................................................

    typename std::unordered_map<K, V>::iterator begin () {
        return m_Content.begin();
    }
    typename std::unordered_map<K, V>::iterator end () {
        return m_Content.end();
    }

    typename std::unordered_map<K, V>::const_iterator begin () const {
        return m_Content.begin();
    }
    typename std::unordered_map<K, V>::const_iterator end () const {
        return m_Content.end();
    }

protected:

    /// The container
    std::unordered_map<K, V> m_Content;
};

// ============================================================================

template <typename K, typename V>
bool Dict<K, V>::empty () const {
    return m_Content.empty();
}

template <typename K, typename V>
size_t Dict<K, V>::size () const {
    return m_Content.size();
}

template <typename K, typename V>
void Dict<K, V>::clear () {
    m_Content.clear();
}

template <typename K, typename V>
bool Dict<K, V>::has (const K& a_Key) const {
    return m_Content.count(a_Key) != 0;
}


template <typename K, typename V>
V& Dict<K, V>::get (const K& a_Key) {
    return m_Content.at(a_Key);
}

template <typename K, typename V>
const V& Dict<K, V>::get (const K& a_Key) const {
    return m_Content.at(a_Key);
}

template <typename K, typename V>
V& Dict<K, V>::get (const K& a_Key, V& a_Default) {
    auto itr = m_Content.find(a_Key);

    if (itr != m_Content.end()) {
        return itr->second;
    }
    else {
        return a_Default;
    }
}

template <typename K, typename V>
const V& Dict<K, V>::get (const K& a_Key, const V& a_Default) const {
    auto itr = m_Content.find(a_Key);

    if (itr != m_Content.end()) {
        return itr->second;
    }
    else {
        return a_Default;
    }
}


template <typename K, typename V>
void Dict<K, V>::set (const K& a_Key, const V& a_Value) {
    m_Content[a_Key] = a_Value;
}


template <typename K, typename V>
void Dict<K, V>::del (const K& a_Key) {
    auto itr = m_Content.find(a_Key);

    if (itr != m_Content.end()) {
        m_Content.erase(itr);
    }
}

template <typename K, typename V>
typename std::unordered_map<K, V>::iterator Dict<K, V>::del (
        const typename std::unordered_map<K, V>::iterator pos)
{
    return m_Content.erase(pos);
}


template <typename K, typename V>
void Dict<K, V>::update (const Dict<K, V>& a_Other) {
    for (const auto& item : a_Other.m_Content) {
        m_Content[item.first] = item.second;
    }
}

// ============================================================================

template <typename K, typename V>
typename std::unordered_map<K, V>::iterator begin (Dict<K, V>& dict) {
    return dict.begin();
}

template <typename K, typename V>
typename std::unordered_map<K, V>::iterator end (Dict<K, V>& dict) {
    return dict.end();
}


template <typename K, typename V>
typename std::unordered_map<K, V>::const_iterator begin (const Dict<K, V>& dict) {
    return dict.begin();
}

template <typename K, typename V>
typename std::unordered_map<K, V>::const_iterator end (const Dict<K, V>& dict) {
    return dict.end();
}

// ============================================================================

#endif // DICT_HH
