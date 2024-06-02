#include "configuration.hh"
#include <algorithm>
#include <cassert>
#include <print>
static auto get_size_suffix_map() -> const std::unordered_map<std::string, unsigned long long> & {
  static bool                                                initialize = true;
  static std::unordered_map<std::string, unsigned long long> map;
  if (initialize) {
    map.insert(std::make_pair("kib", 1024ull));
    map.insert(std::make_pair("kb", 1024ull));
    map.insert(std::make_pair("k", 1024ull));

    map.insert(std::make_pair("mib", 1024ull * 1024));
    map.insert(std::make_pair("mb", 1024ull * 1024));
    map.insert(std::make_pair("m", 1024ull * 1024));

    map.insert(std::make_pair("gib", 1024ull * 1024 * 1024));
    map.insert(std::make_pair("gb", 1024ull * 1024 * 1024));
    map.insert(std::make_pair("g", 1024ull * 1024 * 1024));

    map.insert(std::make_pair("tib", 1024ull * 1024 * 1024 * 1024));
    map.insert(std::make_pair("tb", 1024ull * 1024 * 1024 * 1024));
    map.insert(std::make_pair("t", 1024ull * 1024 * 1024 * 1024));

    map.insert(std::make_pair("pib", 1024ull * 1024 * 1024 * 1024 * 1024));
    map.insert(std::make_pair("pb", 1024ull * 1024 * 1024 * 1024 * 1024));
    map.insert(std::make_pair("p", 1024ull * 1024 * 1024 * 1024 * 1024));
  }
  return map;
}
static auto get_duration_suffix_map() -> const std::unordered_map<std::string, unsigned long long> & {
  static bool                                                initialize = true;
  static std::unordered_map<std::string, unsigned long long> map;
  if (initialize) {
    map.insert(std::make_pair("m", 60ull));
    map.insert(std::make_pair("minute", 60ull));
    map.insert(std::make_pair("minutes", 60ull));

    map.insert(std::make_pair("h", 60ull * 60));
    map.insert(std::make_pair("hour", 60ull * 60));
    map.insert(std::make_pair("hours", 60ull * 60));

    map.insert(std::make_pair("d", 60ull * 60 * 24));
    map.insert(std::make_pair("day", 60ull * 60 * 24));
    map.insert(std::make_pair("days", 60ull * 60 * 24));
  }
  return map;
}

auto Configurations::StaticOptionConfig::parser() const -> parser_t { return this->parser_; }
auto Configurations::StaticOptionConfig::parser(parser_t) -> OptionConfig & {
  throw std::logic_error("cannot modify parser of a common option!");
}
void Configurations::StaticOptionConfig::add_option(Configurations &configuration, std::string name) const {
  configuration.options.emplace_back(std::move(name), this);
}
auto Configurations::DynamicOptionConfig::parser() const -> parser_t { return this->parser_; }
auto Configurations::DynamicOptionConfig::parser(parser_t parser) -> OptionConfig & {
  this->parser_ = parser;
  return *this;
}
void Configurations::DynamicOptionConfig::add_option(Configurations &configuration, std::string name) const {
  configuration.dynamic_configs.emplace_front(*this);
  configuration.options.emplace_back(std::move(name), std::addressof(configuration.dynamic_configs.front()));
}

auto Configurations::CommonParsers::true_parser(
  parser_argument_iterator_t begin, parser_argument_iterator_t end
) -> std::any {
  assert(begin == end);
  return std::make_any<bool>(true);
}
auto Configurations::CommonParsers::duration_parser(
  parser_argument_iterator_t begin, parser_argument_iterator_t end
) -> std::any {
  assert(std::next(begin) == end);
  unsigned long long target;
  size_t             pos = 0;
  try {
    target = std::stoull(*begin, std::addressof(pos));
  } catch (const std::exception &e) {
    std::println(stderr, "invalid value {}", *begin);
    exit(EXIT_FAILURE);
  }
  if (pos != begin->size()) {
    auto suffix = begin->substr(pos);
    std::for_each(suffix.begin(), suffix.end(), [](auto &c) { c = tolower(c); });
    const auto &map  = get_duration_suffix_map();
    auto        iter = map.find(suffix);
    if (iter == map.cend()) {
      std::println(stderr, "invalid suffix {}", begin->substr(pos));
      exit(EXIT_FAILURE);
    }
    target *= iter->second;
  }
  return std::make_any<unsigned long long>(target);
}
auto Configurations::CommonParsers::size_parser(
  parser_argument_iterator_t begin, parser_argument_iterator_t end
) -> std::any {
  assert(std::next(begin) == end);
  unsigned long long target;
  size_t             pos = 0;
  try {
    target = std::stoull(*begin, std::addressof(pos));
  } catch (const std::exception &e) {
    std::println(stderr, "invalid value {}", *begin);
    exit(EXIT_FAILURE);
  }
  if (pos != begin->size()) {
    auto suffix = begin->substr(pos);
    std::for_each(suffix.begin(), suffix.end(), [](auto &c) { c = tolower(c); });
    const auto &map  = get_size_suffix_map();
    auto        iter = map.find(suffix);
    if (iter == map.cend()) {
      std::println(stderr, "invalid suffix {}", begin->substr(pos));
      exit(EXIT_FAILURE);
    }
    target *= iter->second;
  }
  return std::make_any<unsigned long long>(target);
}
auto Configurations::CommonParsers::identity_parser(
  parser_argument_iterator_t begin, parser_argument_iterator_t end
) -> std::any {
  assert(begin != end);
  if (std::distance(begin, end) == 1) {
    return std::make_any<std::string>(*begin);
  } else {
    return std::make_any<std::vector<std::string>>(std::vector<std::string>{begin, end});
  }
}

auto Configurations::parser_dispatcher(
  const Option &option, size_t &break_point, const std::vector<std::string> &args
) -> std::any {
  // till now we are only sure that the name of option is a prefix of current command line item, but can it
  //  be the case that we are looking for --abc but the item is actually --abcdef?
  if (args[break_point] == option.name) {
    // this item is exactly the name, we shall expect something like --abc ...
    // first, find how many items that looks like a parameter can be found
    uint8_t parameters_available = 0;
    for (auto i = break_point + 1; i < args.size(); i++) {
      if (args[i][0] == '-') {
        break;
      }
      parameters_available++;
    }
    // find if parameters SHALL be taken, how many is required
    uint8_t taking_number = option.config->argument_count_ == OptionConfig::VariableArguments
                            ? std::max(parameters_available, static_cast<uint8_t>(1))
                            : option.config->argument_count_;
    if (parameters_available < taking_number) {
      // insufficient parameters
      //  this will only work if the option allows the case that no parameter is being taken
      if (option.config->optional_argument_) {
        // ok, pass an empty range
        return option.config->parser()(args.cbegin(), args.cbegin());
      }
      // no, not working
      std::println(stderr, "insufficient arguments for {}", option.name);
      exit(EXIT_FAILURE);
    }
    auto step = static_cast<parser_argument_iterator_t::difference_type>(++break_point);
    break_point += taking_number;
    return option.config->parser()(
      std::next(args.cbegin(), step),
      std::next(args.cbegin(), step + static_cast<decltype(step)>(taking_number))
    );
  } else if (args[break_point].size() > option.name.size()) {
    // there are some extra part after the name, we have --abc..., what are they?
    if (args[break_point][option.name.size()] == '=') {
      // that is --abc=[...], this works only if the option may take exactly one parameter
      if (
        option.config->optional_argument_ == 1                               // exactly one
        || option.config->argument_count_ == OptionConfig::VariableArguments // variable: one is acceptable
      ) {
        std::vector<std::string> temporary;
        temporary.emplace_back(args[break_point++].substr(option.name.size() + 1));
        return option.config->parser()(temporary.cbegin(), temporary.cend());
      } else {
        // no, this will not work
        std::println(stderr, "invalid option string {} with option name {}", args[break_point], option.name);
        exit(EXIT_FAILURE);
      }
    } else {
      // that is something like --abcdef: not what we should care about, we just care about --abc
      // return an empty std::any
      return {};
    }
  } else {
    // we do not expect the control flow to reach here
    assert(false);
  }
}
auto Configurations::test_option(
  const Option &option, size_t &break_point, const std::vector<std::string> &args
) -> std::any {
  if (args[break_point].substr(0, option.name.size()) == option.name) {
    return Configurations::parser_dispatcher(option, break_point, args);
  }
  // return a empty std::any instance
  return {};
}

void Configurations::add_option(const std::string &name, const parser_t &parser, size_t argument_count) {
  DynamicOptionConfig().parser(parser).argument_count(argument_count).add_option(*this, name);
}
void Configurations::add_option(const std::string &name, const OptionConfig &modifier) {
  modifier.add_option(*this, name);
}

auto Configurations::parse(const std::vector<std::string> &args) const -> Configurations::parse_result_t {
  Configurations::parse_result_t result;
  size_t                         break_point = 1;
  while (break_point < args.size() && args[break_point].substr(0, 2).compare("--") == 0) {
    if (args[break_point].size() == 2) {
      // it is just '--'
      ++break_point;
      break;
    }
    bool matched = false;
    for (const auto &option : this->options) {
      auto &&parse_result = Configurations::test_option(option, break_point, args);
      if (parse_result.has_value()) {
        matched = true;
        result.insert_or_assign(option.name.substr(2), parse_result);
        break;
      }
    }
    if (!matched) {
      // automatically add a implicit option of --help
      if (args[break_point] == "--help") {
        this->help();
        exit(EXIT_SUCCESS);
      }
      std::println(stderr, "unrecognized option {}", args[break_point]);
      this->help();
      exit(EXIT_FAILURE);
    }
  }
  // add break point to result
  result.insert(std::make_pair("_break_point", std::make_any<size_t>(break_point)));

  return result;
}
auto Configurations::parse(int argc, char **argv) const -> Configurations::parse_result_t {
  return this->parse({argv, argv + argc});
}