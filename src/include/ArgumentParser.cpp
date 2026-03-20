#include "ArgumentParser.h"
#include <algorithm>
#include <filesystem>
#include <unordered_set>

namespace argparser {

ArgumentParser::ArgumentParser(int argc, char* argv[]) {
    if (argc < 1 || !argv[0]) {
        throw ArgumentParserException("Invalid command line arguments");
    }

    // Extract executable name from full path
    std::filesystem::path full_path(argv[0]);
    executable_name_ = full_path.filename().string();

    // Store command line arguments (skip program name)
    argv_.reserve(argc - 1);
    for (int i = 1; i < argc; ++i) {
        if (argv[i]) {
            argv_.emplace_back(argv[i]);
        }
    }
}

void ArgumentParser::add_switch(std::string_view name, std::string_view description) {
    add_switch(name, description, SwitchType::FLAG, Requirement::OPTIONAL);
}

void ArgumentParser::add_switch(std::string_view name, std::string_view description, SwitchType type) {
    add_switch(name, description, type, Requirement::OPTIONAL);
}

void ArgumentParser::add_switch(std::string_view name, std::string_view description, 
                                SwitchType type, Requirement requirement) {
    validate_switch_name(name);
    
    const std::string prefix = determine_prefix(name);
    const std::string name_str(name);
    const std::string description_str(description);
    
    switches_.emplace(name_str, SwitchInfo(name_str, description_str, prefix, type, requirement));
}

void ArgumentParser::add_switch_pair(std::string_view short_name, std::string_view long_name,
                                     std::string_view description, SwitchType type, Requirement requirement) {
    validate_switch_name(short_name);
    validate_switch_name(long_name);
    
    const std::string short_prefix = determine_prefix(short_name);
    const std::string long_prefix = determine_prefix(long_name);
    const std::string short_name_str(short_name);
    const std::string long_name_str(long_name);
    const std::string description_str(description);
    
    // Add both switches with pair information
    switches_.emplace(short_name_str, SwitchInfo(short_name_str, description_str, short_prefix, type, requirement));
    switches_.emplace(long_name_str, SwitchInfo(long_name_str, description_str, long_prefix, type, requirement));
    
    // Set up pairing
    switches_.at(short_name_str).pair_name = long_name_str;
    switches_.at(long_name_str).pair_name = short_name_str;
}

void ArgumentParser::parse() {
    if (is_parsed_) {
        return; // Already parsed
    }

    arguments_.clear();
    
    for (size_t i = 0; i < argv_.size(); ++i) {
        const std::string_view arg = argv_[i];
        
        if (is_switch_argument(arg)) {
            i = process_switch_argument(arg, i);
        } else {
            arguments_.emplace_back(arg);
        }
    }
    
    validate_required_switches();
    is_parsed_ = true;
}

std::string ArgumentParser::get_executable_name() const {
    return executable_name_;
}

bool ArgumentParser::is_switch_set(std::string_view name) const {
    const std::string name_str(name);
    auto it = switches_.find(name_str);
    if (it != switches_.end()) {
        return it->second.is_set;
    }
    
    // Check paired switches
    for (const auto& [switch_name, switch_info] : switches_) {
        if (switch_info.pair_name == name_str) {
            return switch_info.is_set;
        }
    }
    
    return false;
}

std::optional<std::string> ArgumentParser::get_switch_value(std::string_view name) const {
    const std::string name_str(name);
    auto it = switches_.find(name_str);
    if (it != switches_.end()) {
        return it->second.value;
    }
    // Check paired switches
    for (const auto& [switch_name, switch_info] : switches_) {
        if (switch_info.pair_name == name_str) {
            return switch_info.value;
        }
    }
    return std::nullopt;
}

std::optional<std::string> ArgumentParser::get_argv_value(size_t index) const {
    if (index < argv_.size()) {
        return argv_[index];
    }
    return std::nullopt;
}

size_t ArgumentParser::get_argument_count() const {
    return arguments_.size();
}

const std::vector<std::string>& ArgumentParser::get_arguments() const {
    return arguments_;
}

void ArgumentParser::print_help(std::string_view header, bool has_non_switch_arguments) const {
    std::cout << "\n" << header << "\n\n";
    std::cout << "Usage:\n  " << executable_name_ << " " << format_switch_line(has_non_switch_arguments) << "\n\n";
    std::cout << "Switches:\n" << format_switch_list();
}

// Private methods

ArgumentParser::SwitchInfo::SwitchInfo(std::string_view name, std::string_view description,
                                      std::string_view prefix, SwitchType type, Requirement requirement)
    : name(name), description(description), prefix(prefix), type(type), requirement(requirement) {}

std::string ArgumentParser::format_switch_list() const {
    std::string result;
    std::unordered_set<std::string> processed_switches;
    
    for (const auto& [name, switch_info] : switches_) {
        // Skip if we've already processed this switch (to avoid duplicates from pairs)
        if (processed_switches.count(name) || (!switch_info.pair_name.empty() && processed_switches.count(switch_info.pair_name))) {
            continue;
        }
        
        if (!switch_info.pair_name.empty()) {
            const auto& paired_switch = switches_.at(switch_info.pair_name);
            // Always display short switch first
            if (switch_info.prefix == "-") {
                result += "  " + switch_info.prefix + switch_info.name + ", " + paired_switch.prefix + paired_switch.name;
            } else {
                result += "  " + paired_switch.prefix + paired_switch.name + ", " + switch_info.prefix + switch_info.name;
            }
        } else {
            result += "  " + switch_info.prefix + switch_info.name;
        }
        
        if (switch_info.type == SwitchType::PARAMETER) {
            result += " <parameter>";
        }
        
        result += "\n  " + switch_info.description;
        
        if (switch_info.requirement == Requirement::REQUIRED) {
            result += " (Required)";
        }
        
        result += "\n";
        
        processed_switches.insert(name);
        if (!switch_info.pair_name.empty()) {
            processed_switches.insert(switch_info.pair_name);
        }
    }
    
    return result;
}

std::string ArgumentParser::format_switch_line(bool has_non_switch_arguments) const {
    const std::string new_line_padding(executable_name_.length() + 3, ' ');
    std::string result;
    std::unordered_set<std::string> processed_switches;
    bool first = true;
    
    for (const auto& [name, switch_info] : switches_) {
        if (processed_switches.count(name) || (!switch_info.pair_name.empty() && processed_switches.count(switch_info.pair_name))) {
            continue;
        }
        
        if (!first) {
            result += " |\n" + new_line_padding;
        }
        first = false;
        
        if (!switch_info.pair_name.empty()) {
            const auto& paired_switch = switches_.at(switch_info.pair_name);
            // Always display short switch first
            if (switch_info.prefix == "-") {
                result += "[" + switch_info.prefix + switch_info.name + " | " + paired_switch.prefix + paired_switch.name;
            } else {
                result += "[" + paired_switch.prefix + paired_switch.name + " | " + switch_info.prefix + switch_info.name;
            }
        } else {
            result += "[" + switch_info.prefix + switch_info.name;
        }
        
        if (switch_info.type == SwitchType::PARAMETER) {
            result += " <parameter>";
        }
        
        result += "]";
        
        processed_switches.insert(name);
        if (!switch_info.pair_name.empty()) {
            processed_switches.insert(switch_info.pair_name);
        }
    }
    
    if (has_non_switch_arguments) {
        if (!result.empty()) {
            result += "\n" + new_line_padding + "<ARGUMENTS>";
        } else {
            result += "<ARGUMENTS>";
        }
    }
    
    return result;
}

std::string ArgumentParser::determine_prefix(std::string_view name) const {
    return (name.length() == 1) ? "-" : "--";
}

void ArgumentParser::validate_switch_name(std::string_view name) const {
    if (name.empty()) {
        throw ArgumentParserException("Switch name cannot be empty");
    }
    
    // Check if switch already exists
    const std::string name_str(name);
    if (switches_.find(name_str) != switches_.end()) {
        throw ArgumentParserException("Switch '" + name_str + "' already exists");
    }
}

bool ArgumentParser::is_switch_argument(std::string_view arg) const {
    return arg.size() > 1 && arg[0] == '-';
}

size_t ArgumentParser::process_switch_argument(std::string_view arg, size_t index) {
    for (auto& [name, switch_info] : switches_) {
        const std::string full_switch_name = switch_info.prefix + switch_info.name;
        
        if (arg == full_switch_name) {
            switch_info.is_set = true;
            
            if (switch_info.type == SwitchType::PARAMETER) {
                return process_parameter_switch(switch_info, index);
            }
            
            update_paired_switch(switch_info);
            return index;
        }
    }
    
    // Unknown switch, treat as argument
    arguments_.emplace_back(arg);
    return index;
}

size_t ArgumentParser::process_parameter_switch(SwitchInfo& switch_info, size_t index) {
    if (index + 1 >= argv_.size()) {
        handle_missing_parameter(switch_info, index);
        return index;
    }
    
    const std::string_view potential_param = argv_[index + 1];
    
    if (is_switch_argument(potential_param)) {
        handle_missing_parameter(switch_info, index);
        return index;
    }
    
    switch_info.value = std::string(potential_param);
    update_paired_switch(switch_info);
    return index + 1;
}

void ArgumentParser::update_paired_switch(const SwitchInfo& switch_info) {
    if (!switch_info.pair_name.empty()) {
        auto& paired_switch = switches_.at(switch_info.pair_name);
        paired_switch.is_set = true;
        paired_switch.value = switch_info.value;
    }
}

void ArgumentParser::validate_required_switches() const {
    for (const auto& [name, switch_info] : switches_) {
        if (switch_info.requirement == Requirement::REQUIRED && !switch_info.is_set) {
            throw ArgumentParserException("Required switch '" + switch_info.prefix + switch_info.name + "' is not set");
        }
    }
}

void ArgumentParser::handle_missing_parameter(const SwitchInfo& switch_info, size_t /* argv_index */) {
    if (switch_info.requirement == Requirement::REQUIRED) {
        throw ArgumentParserException("Missing required parameter for switch '" + 
                                    switch_info.prefix + switch_info.name + "'");
    }
    // Optional parameter missing - leave value as empty optional
}

} // namespace argparser