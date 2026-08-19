/**
 * \file command_registry.ixx
 * \brief Stores and resolves commands available to runtime configuration.
 */
export module command_registry;

import std;

export namespace command_registry {

using Action = std::function<void()>;
using Factory = std::function<Action(std::string_view)>;

class Registry {
public:
    template<typename Function>
        requires std::invocable<Function> &&
                 std::same_as<std::invoke_result_t<Function>, void>
    void add(std::string name, Function function) {
        const auto [entry, inserted] = commands_.emplace(
            std::move(name),
            Action {std::move(function)}
        );

        if (!inserted) {
            throw std::logic_error(
                "Duplicate runtime command: " + entry->first
            );
        }
    }

    void add_factory(
        std::string name,
        Factory factory,
        std::string autocomplete_value = {}
    ) {
        if (autocomplete_value.empty()) {
            autocomplete_value = name + "()";
        }

        const auto [entry, inserted] = factories_.emplace(
            std::move(name),
            FactoryEntry {
                std::move(factory),
                std::move(autocomplete_value)
            }
        );

        if (!inserted) {
            throw std::logic_error(
                "Duplicate runtime command factory: " + entry->first
            );
        }
    }

    [[nodiscard]] std::vector<std::string> registered_names() const {
        std::vector<std::string> names;
        names.reserve(commands_.size() + factories_.size());

        for (const auto& [name, action] : commands_) {
            names.push_back(name);
        }

        for (const auto& [name, entry] : factories_) {
            names.push_back(name);
        }

        std::ranges::sort(names);
        return names;
    }

    [[nodiscard]] std::vector<std::string> autocomplete_values() const {
        std::vector<std::string> values;
        values.reserve(commands_.size() + factories_.size());

        for (const auto& [name, action] : commands_) {
            values.push_back(name);
        }

        for (const auto& [name, entry] : factories_) {
            values.push_back(entry.autocomplete_value);
        }

        std::ranges::sort(values);
        return values;
    }

    [[nodiscard]] Action resolve(std::string_view expression) const {
        expression = trim(expression);
        const std::size_t opening_parenthesis = expression.find('(');

        if (opening_parenthesis == std::string_view::npos) {
            const auto command = commands_.find(std::string {expression});
            return command != commands_.end() ? command->second : nullptr;
        }

        if (expression.empty() || expression.back() != ')') {
            return {};
        }

        const std::string_view name = trim(
            expression.substr(0, opening_parenthesis)
        );
        const auto factory = factories_.find(std::string {name});

        if (factory == factories_.end()) {
            return {};
        }

        const std::string_view arguments = expression.substr(
            opening_parenthesis + 1,
            expression.size() - opening_parenthesis - 2
        );
        return factory->second.resolve(arguments);
    }

private:
    struct FactoryEntry {
        Factory resolve;
        std::string autocomplete_value;
    };

    static std::string_view trim(std::string_view value) {
        const std::size_t first = value.find_first_not_of(" \t");

        if (first == std::string_view::npos) {
            return {};
        }

        const std::size_t last = value.find_last_not_of(" \t");
        return value.substr(first, last - first + 1);
    }

    std::unordered_map<std::string, Action> commands_;
    std::unordered_map<std::string, FactoryEntry> factories_;
};

} // namespace command_registry
