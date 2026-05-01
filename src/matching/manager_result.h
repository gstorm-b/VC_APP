#ifndef MANAGER_RESULT_H
#define MANAGER_RESULT_H

#include <string>

namespace mtc {

struct ManagerResult {
    bool ok = true;
    std::wstring error;

    ManagerResult() = default;

    explicit ManagerResult(bool ok, const std::wstring &error = {})
        : ok(ok), error(error) {}

    static ManagerResult success()
    { return ManagerResult{true}; }

    static ManagerResult fail(const std::wstring &reason)
    { return ManagerResult{false, reason}; }

    /// Implicit bool — lets you write:  if (!result) { ... }
    explicit operator bool() const { return ok; }
};

}

#endif // MANAGER_RESULT_H
