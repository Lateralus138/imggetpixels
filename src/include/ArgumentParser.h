#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>
#include <stdexcept>
#include <iostream>

namespace argparser {

enum class SwitchType {
    FLAG,
    PARAMETER
};

enum class Requirement {
    OPTIONAL,
    REQUIRED
};

class ArgumentParserException : public std::runtime_error {
public:
    explicit ArgumentParserException(const std::string& message) 
        : std::runtime_error(message) {}
};

class ArgumentParser {
public:
    explicit ArgumentParser(int argc, char* argv[]);
    ~ArgumentParser() = default;

    // Delete copy constructor and assignment operator
    ArgumentParser(const ArgumentParser&) = delete;
    ArgumentParser& operator=(const ArgumentParser&) = delete;
    
    // Allow move operations
    ArgumentParser(ArgumentParser&&) = default;
    ArgumentParser& operator=(ArgumentParser&&) = default;

    // Configuration methods
    void add_switch(std::string_view name, std::string_view description);
    void add_switch(std::string_view name, std::string_view description, SwitchType type);
    void add_switch(std::string_view name, std::string_view description, SwitchType type, Requirement requirement);
    void add_switch_pair(std::string_view short_name, std::string_view long_name, 
                        std::string_view description, SwitchType type, Requirement requirement);

    // Parsing
    void parse();
    
    // Query methods
    [[nodiscard]] std::string get_executable_name() const;
    [[nodiscard]] bool is_switch_set(std::string_view name) const;
    [[nodiscard]] std::optional<std::string> get_switch_value(std::string_view name) const;
    [[nodiscard]] std::optional<std::string> get_argv_value(size_t index) const;
    [[nodiscard]] size_t get_argument_count() const;
    [[nodiscard]] const std::vector<std::string>& get_arguments() const;

    // Help system
    void print_help(std::string_view header, bool has_non_switch_arguments = false) const;

private:
    struct SwitchInfo {
        std::string name;
        std::string description;
        std::string prefix;
        std::optional<std::string> value;
        std::string pair_name;
        SwitchType type;
        Requirement requirement;
        bool is_set = false;

        SwitchInfo(std::string_view name, std::string_view description, 
                  std::string_view prefix, SwitchType type, Requirement requirement);
    };

    std::string executable_name_;
    std::vector<std::string> argv_;
    std::vector<std::string> arguments_;
    std::unordered_map<std::string, SwitchInfo> switches_;
    bool is_parsed_ = false;

    [[nodiscard]] std::string format_switch_list() const;
    [[nodiscard]] std::string format_switch_line(bool has_non_switch_arguments) const;
    [[nodiscard]] std::string determine_prefix(std::string_view name) const;
    void validate_switch_name(std::string_view name) const;
    void handle_missing_parameter(const SwitchInfo& switch_info, size_t argv_index);
    
    // New helper methods for parsing
    [[nodiscard]] bool is_switch_argument(std::string_view arg) const;
    size_t process_switch_argument(std::string_view arg, size_t index);
    size_t process_parameter_switch(SwitchInfo& switch_info, size_t index);
    void update_paired_switch(const SwitchInfo& switch_info);
    void validate_required_switches() const;
};

} // namespace argparser

