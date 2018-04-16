#ifndef BASECAMP_LOG_HPP
#define BASECAMP_LOG_HPP

#include <functional>
#include <string>
#include <map>

// Header-Only definition of log functionality for reuse.
namespace basecampLog
{
    /// Severity of log messages.
    enum class Severity
    {
        trace,
        debug,
        info,
        warning,
        error,
        fatal,
    };

    const std::map<Severity, std::string> SeverityText {
        {Severity::trace, "TRACE"},
        {Severity::debug, "DEBUG"},
        {Severity::info, "INFO"},
        {Severity::warning, "WARNING"},
        {Severity::error, "ERROR"},
        {Severity::fatal, "FATAL"},
    };

    /// Definition of a generic log callback.
    using LogCallback = std::function<void(basecampLog::Severity severity, const std::string& message)>;
}

#endif //  BASECAMP_LOG_HPP
