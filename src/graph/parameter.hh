#ifndef GRAPH_PARAMETER_HH
#define GRAPH_PARAMETER_HH

#include <string>
#include <vector>

#include <cstdint>

namespace Graph {

// ============================================================================

class Parameter {
public:

    /// Parameter types
    enum class Type {
        NUMBER,
        CHOICE
    };

    /// Parameter value
    class Value;

    Parameter () = default;

    Parameter (float a_Default, float a_Min, float a_Max,
               float a_Step = 1.0f,
               const std::string& a_Description = std::string());

    Parameter (const std::string& a_Default,
               const std::vector<std::string>& a_Choices,
               const std::string& a_Description = std::string());

    Parameter (const Parameter& ref) = default;

    /// Returns the parameter type
    Type getType () const;
    /// Returns the description
    const std::string& getDescription () const;
    /// Returns true when locked
    bool isLocked () const;

    /// Returns min value
    float getMin  () const;
    /// Returns max value
    float getMax  () const;
    /// Returns the step
    float getStep () const;

    /// Returns a list of available choices
    const std::vector<std::string>& getChoices () const;

    /// Lock / unlock
    void setLock (bool a_Lock);
    /// Sets min value
    void setMin  (float a_Value);
    /// Sets max value
    void setMax  (float a_Value);
    /// Sets step
    void setStep (float a_Value);

    /// Resets to the default value
    void reset ();

    /// Returns the default value
    Value getDefault () const;

    /// Returns current value
    Value get () const;
    /// Sets new value
    void  set (const Value& a_Value);

    /// Value assignment
    Parameter& operator = (const Value& a_Value);

private:

    /// Type
    Type  m_Type = Type::NUMBER;
    /// Default value
    float m_Default = 0.0f;
    /// Current value
    float m_Value = 0.0f;
    /// Locked flag
    bool  m_Locked = false;

    /// Min
    float m_Min  = 0.0f;
    /// Max
    float m_Max  = 0.0f;
    /// Step
    float m_Step = 1.0f;
    /// Available choices
    std::vector<std::string> m_Choices;

    /// Description
    std::string m_Description;
};

// ============================================================================

/// Parameter value
class Parameter::Value {
public:

    /// Types
    enum class Type {
        UNSPEC,
        NUMBER,
        STRING,
        CHOICE,
    };

    /// Default
    Value () = default;
    /// From number
    Value (float a_Value);
    /// From number
    Value (int32_t a_Value);
    /// From string
    Value (const std::string& a_Value);
    /// From string
    Value (const char* a_Value);
    /// From number+string (choice)
    Value (int32_t a_Value, const std::string& a_Choice);

    /// Returns type
    Type getType () const;

    /// Returns value as number
    float asNumber () const;
    /// Returns value as string
    const std::string asString () const;
    /// Returns value as choice
    std::pair<int32_t, std::string> asChoice () const;

protected:

    /// Type
    Type  m_Type   = Type::UNSPEC;
    /// Number value
    float m_Number = 0.0f;
    /// String value
    std::string m_String;
};

// ============================================================================

}; // Graph
#endif // GRAPH_PARAMETER_HH
 
