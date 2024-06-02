#ifndef CONFIGURATION_HH_
#define CONFIGURATION_HH_
#include <any>
#include <cstdint>
#include <cstdio>
#include <forward_list>
#include <functional>
#include <print>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
class Configurations {
public:
  using parser_argument_iterator_t = std::vector<std::string>::const_iterator;
  using pai_t                      = parser_argument_iterator_t;
  using parser_t                   = std::function<std::any(pai_t, pai_t)>;

  struct OptionConfig {
  protected:
    uint8_t argument_count_{0};

  public:
    constexpr static auto VariableArguments = static_cast<decltype(argument_count_)>(-1);
    // set the number of arguments to collect and supply to the parser
    //  a special value, VariableArguments is provided to instruct the tokenizer to collect as much arguments
    //  as possible, that, till the end of command line or some entry starting with '-' is encountered
    inline auto           argument_count(decltype(argument_count_) value) -> OptionConfig           &{
      this->argument_count_ = value;
      return *this;
    }

  protected:
    bool optional_argument_{false};

  public:
    // set if arguments to this option is optional: if no argument is supplied shall be accepted
    //  this option is ignored if argument count is 0
    inline auto optional_argument(decltype(optional_argument_) value) -> OptionConfig & {
      this->optional_argument_ = value;
      return *this;
    }

    OptionConfig() = default;
    constexpr OptionConfig(
      decltype(argument_count_) argument_count, decltype(optional_argument_) optional_argument
    )
      : argument_count_(argument_count), optional_argument_(optional_argument) {}

    [[nodiscard]] virtual auto parser() const -> parser_t                = 0;
    virtual auto               parser(parser_t parser) -> OptionConfig               & = 0;

  protected:
    virtual void add_option(Configurations &configuration, std::string name) const = 0;
    friend Configurations;
  };

  struct StaticOptionConfig : public OptionConfig {
    std::any (*parser_)(pai_t, pai_t);
    template <typename... Args>
    constexpr StaticOptionConfig(decltype(parser_) parser, Args &&...args)
      : OptionConfig(std::forward<Args>(args)...), parser_(parser) {}
    [[nodiscard]] auto parser() const -> parser_t override;
    auto               parser(parser_t parser) -> OptionConfig               &override;

  protected:
    void add_option(Configurations &configuration, std::string name) const override;
  };

  struct DynamicOptionConfig : public OptionConfig {
    parser_t parser_;
    DynamicOptionConfig()          = default;
    virtual ~DynamicOptionConfig() = default;
    [[nodiscard]] auto parser() const -> parser_t override;
    auto               parser(parser_t parser) -> OptionConfig               &override;

  protected:
    void add_option(Configurations &configuration, std::string name) const override;
  };

protected:
  struct Option {
    std::string         name;
    const OptionConfig *config;

    Option(std::string name, const OptionConfig *config) : name(std::move(name)), config(config) {}
  };

  std::vector<Option>                    options;
  std::forward_list<DynamicOptionConfig> dynamic_configs;

  virtual void help() const = 0;

private:
  static auto
  parser_dispatcher(const Option &option, size_t &break_point, const std::vector<std::string> &args)
    -> std::any;
  static auto test_option(const Option &option, size_t &break_point, const std::vector<std::string> &args)
    -> std::any;

public:
  using parse_result_t = std::unordered_map<std::string, std::any>;

  Configurations()          = default;
  virtual ~Configurations() = default;

  // add a new option
  void add_option(const std::string &name, const parser_t &parser, size_t argument_count);
  void add_option(const std::string &name, const OptionConfig &modifier);

  // parse options from a structured vector of strings
  //  the termination point of argument parsing (index of the first non-argument entry) is returned in result
  //   named as _break_point and typed size_t, note the prefixing '_' which is intended to avoid conflicts
  [[nodiscard]] auto parse(const std::vector<std::string> &args) const -> parse_result_t;
  // parse options from raw command line, which is supplied to main as its argument
  //  see also parse(const std::vector<std::string> &args)const
  [[nodiscard]] auto parse(int argc, char **argv) const -> parse_result_t;

  // built-in parsers (and helpers) to simplify parsing of common types
  struct CommonParsers;
};
struct Configurations::CommonParsers {
  CommonParsers() = delete;

  // convert string to unsigned integer, abort if the conversion failed or the string contains part that
  // cannot be converted
  template <typename T> static auto full_convert_unsigned(const std::string &value) -> T {
    size_t             pos = 0;
    unsigned long long result;
    try {
      result = std::stoi(value, std::addressof(pos));
    } catch (const std::exception &error) {
      pos = 0;
    }
    if (value.size() == 0 || pos == 0) {
      std::println(stderr, "cannot convert {}.", value);
      ::exit(EXIT_FAILURE);
    }
    return static_cast<T>(result);
  }
  // produce true without taking any argument
  //  return value made by std::make_any<bool>(...)
  static auto true_parser(parser_argument_iterator_t begin, parser_argument_iterator_t end) -> std::any;
  // produce duration formatted as number of seconds with a single duration specification
  //  return value made by std::make_any<unsigned long long>(...)
  static auto duration_parser(parser_argument_iterator_t begin, parser_argument_iterator_t end) -> std::any;
  // produce size formatted as number of bytes with a single size specification
  //  return value made by std::make_any<unsigned long long>(...)
  static auto size_parser(parser_argument_iterator_t begin, parser_argument_iterator_t end) -> std::any;
  // produce result as is, that is store the input argument directly
  //  if there is only one argument supplied, return value made by std::make_any<std::string>
  //  otherwise, return value made by std::make_any<std::vector<std::string>>
  static auto identity_parser(parser_argument_iterator_t begin, parser_argument_iterator_t end) -> std::any;

  constexpr static StaticOptionConfig true_parser_option{true_parser, 0, false};
  constexpr static StaticOptionConfig duration_parser_option{duration_parser, 1, false};
  constexpr static StaticOptionConfig size_parser_option{size_parser, 1, false};
};
#endif