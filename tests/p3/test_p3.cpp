// tests/p3/test_p3.cpp — YOUR P3 TEST SUITE.
//
// The five tests below are worked examples. Keep them, and add your own
// until you have AT LEAST 15 (spec §6 lists what must be covered).
// Name every test <area>_<what>: reg_ for the registry, ws_ for the
// workspace, calc_ for the calculator.
//
// Build & run:   cmake --build build && ./build/test_p3
// One area:      ./build/test_p3 reg_        (names starting with "reg_")
// One test:      ./build/test_p3 =reg_add_and_find

#include "../mini_test.h"
#include "../test_helpers.h"     // TempDir, write_text, args1, ...

#include "tools/calculator.h"
#include "tools/tool_registry.h"
#include "tools/workspace.h"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <set>

namespace fs = std::filesystem;

namespace {

// A tool whose name you choose — use it to fill the registry in tests.
class NamedTool : public Tool {
public:
    explicit NamedTool(std::string n) : n_(std::move(n)), schema_(JsonValue::object()) {}
    std::string name() const override { return n_; }
    std::string description() const override { return "test tool"; }
    const JsonValue& schema() const override { return schema_; }
    ToolResult invoke(const JsonValue&, ToolContext&) override { return ToolResult::success(n_); }
private:
    std::string n_;
    JsonValue   schema_;   // an empty JSON object: {}
};

// Tokenize with the provided tokenizer, then run YOUR calc_evaluate.
bool eval(const std::string& expr, double* value, std::string* error) {
    std::vector<CalcToken> tokens;
    if (!calc_tokenize(expr, &tokens, error)) return false;
    return calc_evaluate(tokens, value, error);
}

}  // namespace

// ---------------------------------------------------------------- examples

TEST(reg_add_and_find) {
    ToolRegistry r;
    auto t = std::make_unique<NamedTool>("alpha");
    Tool* raw = t.get();                        // remember the address before moving
    REQUIRE(r.add(std::move(t)));
    CHECK(r.size() == 1);
    CHECK(r.find("alpha") == raw);              // same object, not a copy
    CHECK(r.find("beta") == nullptr);
}

TEST(reg_duplicate_keeps_original) {
    ToolRegistry r;
    auto first = std::make_unique<NamedTool>("alpha");
    Tool* original = first.get();
    REQUIRE(r.add(std::move(first)));
    CHECK(!r.add(std::make_unique<NamedTool>("alpha")));
    CHECK(r.find("alpha") == original);
    CHECK(r.size() == 1);
}

TEST(ws_rejects_parent_escape) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    REQUIRE(ws.resolve("notes/todo.txt", &out, &reason));   // sanity: accepts *something*
    CHECK(out == ws.root() / "notes" / "todo.txt");
    CHECK(!ws.resolve("../etc/passwd", &out, &reason));
    CHECK(!reason.empty());
}

TEST(calc_precedence) {
    double v = 0;
    std::string err;
    REQUIRE(eval("2 + 3 * 4", &v, &err));
    CHECK(v == 14);
}

TEST(calc_division_by_zero) {
    double v = 0;
    std::string err;
    CHECK(!eval("1 / 0", &v, &err));
    CHECK(err == "division by zero");
}


TEST(reg_null_rejected) {
    ToolRegistry r;
    CHECK(!r.add(nullptr));
    CHECK(r.size() == 0);
}

TEST(reg_for_each_visits_each_once) {
    ToolRegistry r;
    REQUIRE(r.add(std::make_unique<NamedTool>("alpha")));
    REQUIRE(r.add(std::make_unique<NamedTool>("beta")));
    REQUIRE(r.add(std::make_unique<NamedTool>("gamma")));

    std::set<std::string> names;
    int visits = 0;

    r.for_each([&](const Tool& tool) {
        ++visits;
        names.insert(tool.name());
    });

    CHECK(visits == 3);
    CHECK(names.size() == 3);
    CHECK(names.count("alpha") == 1);
    CHECK(names.count("beta") == 1);
    CHECK(names.count("gamma") == 1);
}

TEST(reg_10000_tools) {
    ToolRegistry r;
    Tool* first = nullptr;

    for (int i = 0; i < 10000; ++i) {
        auto tool = std::make_unique<NamedTool>(
            "tool_" + std::to_string(i));

        if (i == 0) first = tool.get();

        REQUIRE(r.add(std::move(tool)));
        CHECK(r.load_factor() <= 1.0);
    }

    CHECK(r.size() == 10000);
    CHECK(r.find("tool_0") == first);
    CHECK(r.max_probe_length() <= 64);

    for (int i = 0; i < 10000; ++i) {
        CHECK(r.find("tool_" + std::to_string(i)) != nullptr);
    }

    std::cout << "Buckets: " << r.bucket_count() << "\n";
    std::cout << "Load factor: " << r.load_factor() << "\n";
    std::cout << "Longest chain: " << r.max_probe_length() << "\n";
}

TEST(reg_alternative_hash) {
    ToolRegistry r;

    for (int i = 0; i < 10000; ++i) {
        REQUIRE(r.add(std::make_unique<NamedTool>(
            "tool_" + std::to_string(i))));
    }

    std::vector<int> counts(r.bucket_count(), 0);

    for (int i = 0; i < 10000; ++i) {
        std::string name = "tool_" + std::to_string(i);

        std::size_t weak_hash =
            static_cast<unsigned char>(name[0]) + name.size();

        ++counts[weak_hash % counts.size()];
    }

    int weak_longest = *std::max_element(
        counts.begin(), counts.end());

    std::cout << "FNV-1a longest chain: "
              << r.max_probe_length() << "\n";
    std::cout << "Weak hash longest chain: "
              << weak_longest << "\n";

    CHECK(weak_longest > static_cast<int>(r.max_probe_length()));
}

TEST(ws_normalizes_paths) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;

    REQUIRE(ws.resolve("./notes//todo.txt", &out, &reason));
    CHECK(out == ws.root() / "notes" / "todo.txt");

    REQUIRE(ws.resolve("a/../b", &out, &reason));
    CHECK(out == ws.root() / "b");

    REQUIRE(ws.resolve("a/..", &out, &reason));
    CHECK(out == ws.root());
}

TEST(ws_accepts_root_and_new_files) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;

    REQUIRE(ws.resolve(".", &out, &reason));
    CHECK(out == ws.root());

    REQUIRE(ws.resolve("new/dir/file.txt", &out, &reason));
    CHECK(out == ws.root() / "new/dir/file.txt");

    REQUIRE(ws.resolve("my..notes.txt", &out, &reason));
    CHECK(out == ws.root() / "my..notes.txt");
}

TEST(ws_rejects_empty_and_null) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;

    CHECK(!ws.resolve("", &out, &reason));
    CHECK(!reason.empty());

    std::string bad = std::string("notes") + '\0' + "/x";
    reason.clear();

    CHECK(!ws.resolve(bad, &out, &reason));
    CHECK(!reason.empty());
}

TEST(ws_rejects_absolute_and_escape) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;

    std::vector<std::string> bad_paths = {
        "/etc/passwd",
        "..",
        "../etc/passwd",
        "a/../../b",
        "notes/../../ws/notes/todo.txt"
    };

    for (const auto& path : bad_paths) {
        reason.clear();
        CHECK(!ws.resolve(path, &out, &reason));
        CHECK(!reason.empty());
    }
}

TEST(calc_left_to_right) {
    double v = 0;
    std::string err;

    REQUIRE(eval("10 - 4 - 3", &v, &err));
    CHECK(v == 3);

    REQUIRE(eval("20 / 2 / 2", &v, &err));
    CHECK(v == 5);
}

TEST(calc_parentheses) {
    double v = 0;
    std::string err;

    REQUIRE(eval("(2 + 3) * 4", &v, &err));
    CHECK(v == 20);

    REQUIRE(eval("2 * (3 + 4) - 5", &v, &err));
    CHECK(v == 9);
}

TEST(calc_negatives_and_decimals) {
    double v = 0;
    std::string err;

    REQUIRE(eval("-3 * -3", &v, &err));
    CHECK(v == 9);

    REQUIRE(eval("7 / 2", &v, &err));
    CHECK(v == 3.5);
}

TEST(calc_additional_expressions) {
    double v = 0;
    std::string err;

    REQUIRE(eval("8 * (2 + 3) - 10", &v, &err));
    CHECK(v == 30);

    REQUIRE(eval("(12 - 4) / (1 + 3)", &v, &err));
    CHECK(v == 2);
}


int main(int argc, char** argv) { return mt::run_all(argc, argv); }
