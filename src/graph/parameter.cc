#include "exception.hh"
#include "parameter.hh"

#include <utils/exception.hh>
#include <stringf.hh>

#include <stdexcept>
#include <algorithm>

#include <cmath>

namespace Graph {

// ============================================================================

Parameter::Parameter (float a_Default, float a_Min, float a_Max,
                      float a_Step,
                      const std::string& a_Description) :

    m_Type        (Type::NUMBER),
    m_Default     (a_Default),
    m_Value       (a_Default),
    m_Locked      (false),
    m_Min         (a_Min),
    m_Max         (a_Max),
    m_Step        (a_Step),
    m_Description (a_Description)
{
    // Round
    m_Value = roundf(m_Value / m_Step) * m_Step;
}

Parameter::Parameter (const std::string& a_Default,
                      const std::vector<std::string>& a_Choices,
                      const std::string& a_Description) :

    m_Type        (Type::CHOICE),
    m_Locked      (false),
    m_Choices     (a_Choices),
    m_Description (a_Description)
{
    // Find the default choice on the list
    auto it = std::find(m_Choices.begin(), m_Choices.end(), a_Default);
    if (it == m_Choices.end()) {
        THROW(ParameterError,
            "The default choice '%s' not on the choice list!", a_Default.c_str());
    }

    m_Default = std::distance(m_Choices.begin(), it);
    m_Value   = m_Default;

    m_Min   = 0.0f;
    m_Max   = (m_Choices.size() - 1);
    m_Step  = 1.0f;
}

// ============================================================================

Parameter::Type Parameter::getType () const {
    return m_Type;
}

const std::string& Parameter::getDescription () const {
    return m_Description;
}

bool Parameter::isLocked () const {
    return m_Locked;
}

float Parameter::getMin () const {
    return m_Min;
}

float Parameter::getMax () const {
    return m_Max;
}

float Parameter::getStep () const {
    return m_Step;
}

const std::vector<std::string>& Parameter::getChoices () const {
    return m_Choices;
}

// ============================================================================

void Parameter::setLock (bool a_Lock) {
    m_Locked = a_Lock;
}

void Parameter::setMin (float a_Value) {

    // Cannot set on a choice parameter
    if (m_Type != Type::NUMBER) {
        THROW(ParameterError, "Cannot set min of a non-number parameter");
    }

    // Round to step
    a_Value = floorf(a_Value / m_Step) * m_Step;

    // Update others
    if (m_Max < a_Value) {
        m_Max = a_Value;
    }
    if (m_Value < a_Value) {
        m_Value = a_Value;
    }
    if (m_Default < a_Value) {
        m_Default = a_Value;
    }

    // Update
    m_Min = a_Value;
}

void Parameter::setMax (float a_Value) {

    // Cannot set on a choice parameter
    if (m_Type != Type::NUMBER) {
        THROW(ParameterError, "Cannot set max of a non-number parameter");
    }

    // Round to step
    a_Value = ceilf(a_Value / m_Step) * m_Step;

    // Update others
    if (m_Min > a_Value) {
        m_Min = a_Value;
    }
    if (m_Value > a_Value) {
        m_Value = a_Value;
    }
    if (m_Default > a_Value) {
        m_Default = a_Value;
    }

    // Update
    m_Max = a_Value;
}

void Parameter::setStep (float a_Value) {

    // Cannot set on a choice parameter
    if (m_Type != Type::NUMBER) {
        THROW(ParameterError, "Cannot set step of a non-number parameter");
    }

    // Update
    m_Step = a_Value;

    // Round all
    m_Min = floorf(m_Min / m_Step) * m_Step;
    m_Max = ceilf (m_Max / m_Step) * m_Step;
    
    m_Value   = roundf(m_Value   / m_Step) * m_Step;
    m_Default = roundf(m_Default / m_Step) * m_Step;
}

void Parameter::reset () {
    set(m_Default);
}

// ============================================================================

Parameter::Value Parameter::getDefault () const {

    switch (m_Type)
    {
    case Parameter::Type::NUMBER:
        return Value(m_Default);

    case Parameter::Type::CHOICE:
        size_t idx = m_Default;
        return Value(idx, m_Choices[idx]);
    }

    return Parameter::Value();
}

// ============================================================================

Parameter::Value Parameter::get () const {

    switch (m_Type)
    {
    case Parameter::Type::NUMBER:
        return Value(m_Value);

    case Parameter::Type::CHOICE:
        size_t idx = m_Value;
        return Value(idx, m_Choices[idx]);
    }

    return Parameter::Value();
}

void Parameter::set (const Value& a_Value) {

    // Assigning a number
    if (a_Value.getType() == Value::Type::NUMBER) {
        float value = a_Value.asNumber();

        // Check range
        if (value < m_Min || value > m_Max) {
            THROW(ParameterError, "Value out of range (%.3f - %.3f)",
                m_Min, m_Max);
        }

        // Assign and round
        m_Value = roundf(value / m_Step) * m_Step;
    }

    // Assigning a string
    else if (a_Value.getType() == Value::Type::STRING) {
        const std::string& string = a_Value.asString();

        // A string can only be assignet to a CHOICE parameter
        if (m_Type != Type::CHOICE) {
            throw ParameterError("Cannot assing a string to a non-choice parameter");
        }

        // Check if the choice is legal
        auto it = std::find(m_Choices.begin(), m_Choices.end(), string);
        if (it == m_Choices.end()) {
            THROW(ParameterError, "Illegal choice '%s'", string.c_str());
        }

        // Assign
        m_Value = std::distance(m_Choices.begin(), it);
    }

    // Assigning a choice
    else if (a_Value.getType() == Value::Type::CHOICE) {
        const auto& choice = a_Value.asChoice();

        // A choice can only be assignet to a CHOICE parameter
        if (m_Type != Type::CHOICE) {
            throw ParameterError("Cannot assing a choice to a non-choice parameter");
        }

        // Check if the choice is legal
        auto it = std::find(m_Choices.begin(), m_Choices.end(), choice.second);
        if (it == m_Choices.end()) {
            THROW(ParameterError, "Illegal choice '%s'", choice.second.c_str());
        }

        // Check if the index matches the choice
        size_t idx = std::distance(m_Choices.begin(), it);
        if (idx != (size_t)choice.first) {
            THROW(ParameterError, "Choice '%s' and index '%d' do not match",
                choice.second.c_str(),
                choice.first
                );
        }

        // Assign
        m_Value = idx;
     }
}

Parameter& Parameter::operator = (const Value& a_Value) {
    set(a_Value);
    return *this;
}

// ============================================================================

Parameter::Value::Value (float a_Value) :
    m_Type   (Type::NUMBER),
    m_Number (a_Value)
{
}

Parameter::Value::Value (int32_t a_Value) :
    m_Type   (Type::NUMBER),
    m_Number (a_Value)
{
}

Parameter::Value::Value (const std::string& a_Value) :
    m_Type   (Type::STRING),
    m_String (a_Value)
{
}

Parameter::Value::Value (const char* a_Value) :
    m_Type   (Type::STRING),
    m_String (a_Value)
{
}

Parameter::Value::Value (int32_t a_Value, const std::string& a_Choice) :
    m_Type   (Type::CHOICE),
    m_Number (a_Value),
    m_String (a_Choice)
{
}


Parameter::Value::Type Parameter::Value::getType () const {
    return m_Type;
}


float Parameter::Value::asNumber () const {
    if (m_Type == Type::STRING) {
        throw ParameterError("Cannot convert string value to a number");
    }

    return m_Number;
}

const std::string Parameter::Value::asString () const {
    if (m_Type == Type::NUMBER) {
        return stringf("%.3f", m_Number);
    }
    if (m_Type == Type::CHOICE) {
        return stringf("%s(%d)", m_String.c_str(), (int32_t)m_Number);
    }

    return m_String;
}

std::pair<int, std::string> Parameter::Value::asChoice () const {
    if (m_Type != Type::CHOICE) {
        throw ParameterError("Cannot convert to a choice value");
    }

    return std::make_pair((int32_t)m_Number, m_String);
}

// ============================================================================

}; // Graph
