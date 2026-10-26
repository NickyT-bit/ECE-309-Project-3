
#include "tools/workspace.h"

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

bool Workspace::resolve(std::string_view user_path,
                        fs::path* out,
                        std::string* reason) const {

    // Reject empty paths.
    if (user_path.empty()) {
        *reason = "empty path";
        return false;
    }

    // Reject paths containing null characters.
    if (user_path.find('\0') != std::string_view::npos) {
        *reason = "path contains null character";
        return false;
    }

    // Convert the input into a filesystem path.
    fs::path p{std::string(user_path)};

    // Absolute paths are not allowed.
    if (p.is_absolute() || p.has_root_path()) {
        *reason = "absolute path not allowed";
        return false;
    }

    // Remove unnecessary . and .. components.
    fs::path normalized = p.lexically_normal();

    // Reject paths that escape the workspace.
    if (!normalized.empty() && *normalized.begin() == "..") {
        *reason = "path escapes workspace";
        return false;
    }

    // Build the final path inside the workspace.
    if (normalized == ".") {
        *out = root();
    } else {
        *out = root() / normalized;
    }

    return true;
}
