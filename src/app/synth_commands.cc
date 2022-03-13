#include "synth_app.hh"

#include <utils/utils.hh>
#include <graph/exception.hh>

#include <strutils.hh>
#include <stringf.hh>

#include <algorithm>

// ============================================================================

std::vector<std::string> SynthApp::cmdClearInstruments (const std::vector<std::string>& a_Args) {
    std::vector<std::string> response;

    // Check syntax
    if (a_Args.size() != 1) {
        response.push_back("ERR:Invalid syntax");
        return response;
    }

    // Clear instruments
    try {
        deleteInstruments();
    }

    catch (const std::runtime_error& ex) {
        response.push_back(std::string("ERR:") + ex.what());
        return response;
    }

    // Clear stored config file names
    m_ConfigFiles.clear();

    // Success
    response.push_back("OK");
    return response;
}

std::vector<std::string> SynthApp::cmdLoadInstruments (const std::vector<std::string>& a_Args) {
    std::vector<std::string> response;

    // Check syntax
    if (a_Args.size() != 2) {
        response.push_back("ERR:Invalid syntax");
        return response;
    }

    // Check if we already have that config file loaded
    if (m_ConfigFiles.count(a_Args[1])) {
        response.push_back("ERR:Already loaded");
        return response;
    }

    // Load instruments
    try {
        loadInstruments(a_Args[1]);
    }

    catch (const std::runtime_error& ex) {
        response.push_back(std::string("ERR:") + ex.what());
        return response;
    }

    // Success
    response.push_back("OK");
    return response;
}

std::vector<std::string> SynthApp::cmdReloadInstruments (const std::vector<std::string>& a_Args) {
    std::vector<std::string> response;

    // Check syntax
    if (a_Args.size() != 1) {
        response.push_back("ERR:Invalid syntax");
        return response;
    }

    try {

        // Delete all
        deleteInstruments();

        // Loop over config files and try loading them
        for (auto& configFile : m_ConfigFiles) {
            loadInstruments(configFile);
        }
    }

    catch (const std::runtime_error& ex) {
        response.push_back(std::string("ERR:") + ex.what());
        return response;
    }

    // Success
    response.push_back("OK");
    return response;
}

std::vector<std::string> SynthApp::cmdDumpInstruments (const std::vector<std::string>& a_Args) {
    std::vector<std::string> response;

    // Check syntax
    if (a_Args.size() != 1) {
        response.push_back("ERR:Invalid syntax");
        return response;
    }

    try {

        // Dump instrument graphs
        dumpInstruments();
    }

    catch (const std::runtime_error& ex) {
        response.push_back(std::string("ERR:") + ex.what());
        return response;
    }

    // Success
    response.push_back("OK");
    return response;
}

// ============================================================================

std::vector<std::string> SynthApp::cmdListParams (const std::vector<std::string>& a_Args) {
    std::vector<std::string> response;

    // Check syntax
    if (a_Args.size() != 1) {
        response.push_back("ERR:Invalid syntax");
        return response;
    }

    // Sorted instrument name list
    std::vector<std::string> instrumentNames;
    for (auto& it : m_Instruments) {
        instrumentNames.push_back(it.first);
    }
    std::sort(instrumentNames.begin(), instrumentNames.end());

    // For each syntesizer
    for (auto& instrumentName : instrumentNames) {
        auto& instr  = m_Instruments.get(instrumentName);
        auto& params = instr->getParameters();

        // Sorted parameter name list
        std::vector<std::string> paramNames;
        for (auto& it : params)
            paramNames.push_back(it.first);
        std::sort(paramNames.begin(), paramNames.end());

        // List parameters
        for (auto& paramName : paramNames) {
            auto& param = params.get(paramName);

            // Skip locked parameters
            if (param.isLocked()) {
                continue;
            }

            // Format the specification string
            std::vector<std::string> fields;
            fields.push_back(stringf("%s.%s", instrumentName.c_str(), paramName.c_str()));
            fields.push_back(param.get().asString());

            if (param.getType() == Graph::Parameter::Type::NUMBER) {
                fields.push_back("NUMBER");
                fields.push_back(stringf("%.3f", param.getMin()));
                fields.push_back(stringf("%.3f", param.getMax()));
                fields.push_back(stringf("%.3f", param.getStep()));
            }

            if (param.getType() == Graph::Parameter::Type::CHOICE) {
                fields.push_back("CHOICE");
                fields.push_back(strutils::join(";", param.getChoices()));
            }

            fields.push_back(param.getDescription());

            response.push_back(strutils::join(",", fields));
        }
    }

    // Success
    response.push_back("OK");
    return response;
}

std::vector<std::string> SynthApp::cmdSetParam (const std::vector<std::string>& a_Args) {
    std::vector<std::string> response;

    // Syntax
    if (a_Args.size() != 3) {
        response.push_back("ERR:Invalid syntax");
        return response;
    }

    // Get the target instrument
    std::vector<std::string> fields = strutils::split(a_Args[1], ".", 1);
    if (fields.size() != 2) {
        response.push_back("ERR:Invalid parameter specification");
        return response;
    }

    if (!m_Instruments.has(fields[0])) {
        response.push_back(stringf("ERR:No instrument '%s'", fields[0].c_str()));
        return response;
    }

    auto& instrument = m_Instruments.get(fields[0]);

    // Get the value
    Dict<std::string, Graph::Parameter::Value> params;
    if (Utils::isFloat(a_Args[2])) {
        params.set(fields[1], (float)atof(a_Args[2].c_str()));
    } else {
        params.set(fields[1], a_Args[2]);
    }

    // Try setting the parameter
    try {
        instrument->updateParameters(params);
    }

    catch (const Graph::ParameterError& ex) {
        response.push_back(std::string("ERR:") + ex.what());
        return response;
    }

    // Success
    response.push_back("OK");
    return response;
}

std::vector<std::string> SynthApp::cmdGetParam (const std::vector<std::string>& a_Args) {
    std::vector<std::string> response;

    // Syntax
    if (a_Args.size() != 2) {
        response.push_back("ERR:Invalid syntax");
        return response;
    }

    // Get the target instrument
    std::vector<std::string> fields = strutils::split(a_Args[1], ".", 1);
    if (fields.size() != 2) {
        response.push_back("ERR:Invalid parameter specification");
        return response;
    }

    if (!m_Instruments.has(fields[0])) {
        response.push_back(stringf("ERR:No instrument '%s'", fields[0].c_str()));
        return response;
    }

    auto& instrument = m_Instruments.get(fields[0]);

    // Get the parameter
    auto& params = instrument->getParameters();
    if (!params.has(fields[1])) {
        response.push_back(stringf("ERR:Parameter '%s' not found", fields[1].c_str()));
        return response;
    }

    // Get the parameter value
    auto& param = params.get(fields[1]);
    response.push_back(param.get().asString());

    // Success
    response.push_back("OK");
    return response;
}

std::vector<std::string> SynthApp::cmdSaveParams (const std::vector<std::string>& a_Args) {
    std::vector<std::string> response;

    // Check syntax
    if (a_Args.size() != 1 && a_Args.size() != 2) {
        response.push_back("ERR:Invalid syntax");
        return response;
    }

    if (a_Args.size() == 2) {
        saveParameters(a_Args[1]);
    } else {
        saveParameters();
    }

    // Success
    response.push_back("OK");
    return response;
}

std::vector<std::string> SynthApp::cmdLoadParams (const std::vector<std::string>& a_Args) {
    std::vector<std::string> response;

    // Check syntax
    if (a_Args.size() != 1 && a_Args.size() != 2) {
        response.push_back("ERR:Invalid syntax");
        return response;
    }

    if (a_Args.size() == 2) {

        // Check if the file exist
        FILE* fp = nullptr;
        if ((fp = fopen(a_Args[1].c_str(), "r")) != nullptr) {
            fclose(fp);
            loadParameters(a_Args[1]);
        }
        else {
            response.push_back(stringf("ERR:File '%s' not found!", a_Args[1].c_str()));
            return response;
        }

    } else {
        loadParameters();
    }

    // Success
    response.push_back("OK");
    return response;
}

std::vector<std::string> SynthApp::cmdResetParams (const std::vector<std::string>& a_Args) {
    std::vector<std::string> response;

    // Check syntax
    if (a_Args.size() != 1) {
        response.push_back("ERR:Invalid syntax");
        return response;
    }

    // For each instrument
    for (auto& itr : m_Instruments) {
        auto& instr  = itr.second;
        auto& params = instr->getParameters();

        // Get parameters names and default values
        Graph::Module::ParameterValues defaults;
        for (auto& it : params) {
            auto& param = it.second;
            if (!param.isLocked()) {
                defaults.set(it.first, param.getDefault());
            }
        }

        // Update all of them
        try {
            instr->updateParameters(defaults);
        }

        catch (const Graph::ParameterError& ex) {
            response.push_back(std::string("ERR:") + ex.what());
            return response;
        }
    }

    // Success
    response.push_back("OK");
    return response;
}

// ============================================================================

std::vector<std::string> SynthApp::cmdRecord (const std::vector<std::string>& a_Args) {
    std::vector<std::string> response;

    // Syntax
    if (a_Args.size() != 2) {
        response.push_back("ERR:Invalid syntax");
        return response;
    }

    // Start recording
    if (a_Args[1] == "start") {

        // Already running
        if (m_Recorder->isRecording()) {
            response.push_back("ERR:Already running");
        }

        // Start
        else {
            bool res = m_Recorder->start();
            if (res) {
                response.push_back(m_Recorder->getFileName());
                response.push_back("OK");
            }
            else {
                response.push_back("ERR:Error starting recording");
            }
        }
    }
    // Stop recording
    else if (a_Args[1] == "stop") {

        // Already stopped
        if (!m_Recorder->isRecording()) {
            response.push_back("ERR:Already stopped");
        }

        // Stop
        else {
            m_Recorder->stop();
            response.push_back("OK");
        }
    }
    // Get status
    else if (a_Args[1] == "status") {
        if (m_Recorder->isRecording()) {
            response.push_back("running");
            response.push_back(m_Recorder->getFileName());
        }
        else {
            response.push_back("stopped");
        }

        response.push_back("OK");
    }
    // Invalid argument
    else {
        response.push_back(stringf("ERR:Invalid argument '%s'",
            a_Args[1].c_str()));
    }

    return response;
}

// ============================================================================

std::vector<std::string> SynthApp::processCommand (const std::string& a_Command,
                                                   int a_ClientId)
{
    (void)a_ClientId;

    std::vector<std::string> response;

    // Split the command
    auto args = strutils::split(a_Command);

    // Empty line
    if (args.empty()) {
        return response;
    }

    // Execute command
    if (args[0] == "load_instruments") {
        return cmdLoadInstruments(args);
    }
    else if (args[0] == "reload_instruments") {
        return cmdReloadInstruments(args);
    }
    else if (args[0] == "clear_instruments") {
        return cmdClearInstruments(args);
    }
    else if (args[0] == "list_params") {
        return cmdListParams(args);
    }
    else if (args[0] == "set_param") {
        return cmdSetParam(args);
    }
    else if (args[0] == "get_param") {
        return cmdGetParam(args);
    }
    else if (args[0] == "load_params") {
        return cmdLoadParams(args);
    }
    else if (args[0] == "save_params") {
        return cmdSaveParams(args);
    }
    else if (args[0] == "reset_params") {
        return cmdResetParams(args);
    }
    else if (args[0] == "record") {
        return cmdRecord(args);
    }

    // Unknown command
    else {
        response.push_back("ERR:Unknown command");
    }

    return response;
}
