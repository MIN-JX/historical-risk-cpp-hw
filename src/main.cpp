#include "risk/analytics.hpp"

#include <charconv>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void help() {
    std::cout <<
        "Empirical loss Value at Risk and fractional-tail Expected Shortfall.\n"
        "Usage: risk_analytics --input FILE [option value ...]\n\n"
        "  --input FILE           One-column CSV headed loss (required)\n"
        "  --confidence-bps N     Confidence, integer 1..9999 (default 9500)\n"
        "  --method NAME          sort or partition (default partition)\n"
        "  --help                 Show this help; use alone\n\n"
        "Positive observations are losses; negative observations are profits.\n"
        "9500 basis points means 95% confidence. Values use the input currency.\n"
        "VaR uses the nearest-rank empirical quantile. Expected Shortfall averages\n"
        "exactly the worst (1-confidence) sample mass, including a fractional\n"
        "boundary observation. Negative risk estimates are not clamped to zero.\n"
        "CSV accepts finite decimal numbers, optional UTF-8 BOM and CRLF lines.\n"
        "Empty rows, quoted fields, multiple columns and nonfinite values fail.\n"
        "At most 1000000 observations are accepted. Results are metric,value CSV\n"
        "on standard output; errors go to standard error and exit with status 1.\n";
}

bool known_option(const std::string& option) {
    return option == "--input" || option == "--confidence-bps" || option == "--method";
}

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n\f\v");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n\f\v");
    return text.substr(first, last - first + 1);
}

unsigned confidence(const std::string& text) {
    if (text.empty()) throw std::invalid_argument("confidence requires decimal digits");
    for (char digit : text) {
        if (digit < '0' || digit > '9') {
            throw std::invalid_argument("confidence requires decimal digits only");
        }
    }
    unsigned result = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() ||
        result == 0 || result >= 10000) {
        throw std::invalid_argument("confidence must be between 1 and 9999 basis points");
    }
    return result;
}

double decimal(const std::string& text, std::size_t line_number) {
    const std::string value = trim(text);
    if (value.empty() || value.find_first_of(",\"") != std::string::npos) {
        throw std::invalid_argument("invalid one-column loss at line " + std::to_string(line_number));
    }
    // Validate decimal grammar explicitly: strtod also accepts hexadecimal and
    // nonfinite spellings, which are outside this one-column CSV contract.
    std::size_t index = 0;
    if (value[index] == '+' || value[index] == '-') ++index;
    std::size_t digits = 0;
    while (index < value.size() && value[index] >= '0' && value[index] <= '9') {
        ++index;
        ++digits;
    }
    if (index < value.size() && value[index] == '.') {
        ++index;
        while (index < value.size() && value[index] >= '0' && value[index] <= '9') {
            ++index;
            ++digits;
        }
    }
    bool valid = digits != 0;
    if (index < value.size() && (value[index] == 'e' || value[index] == 'E')) {
        ++index;
        if (index < value.size() && (value[index] == '+' || value[index] == '-')) ++index;
        const std::size_t exponent_start = index;
        while (index < value.size() && value[index] >= '0' && value[index] <= '9') ++index;
        valid = valid && index != exponent_start;
    }
    valid = valid && index == value.size();
    errno = 0;
    char* end = nullptr;
    // The standalone CLI does not change the C locale from its initial "C"
    // locale. Unlike stream extraction, strtod permits representable subnormals
    // even when the implementation reports ERANGE for them.
    const double parsed = valid ? std::strtod(value.c_str(), &end) : 0.0;
    if (!valid || end != value.c_str() + value.size() || !std::isfinite(parsed) ||
        (errno == ERANGE && parsed == 0.0)) {
        throw std::invalid_argument("expected a finite decimal loss at line " +
                                    std::to_string(line_number));
    }
    return parsed;
}

std::vector<double> read_losses(const std::string& filename) {
    if (filename.empty()) throw std::invalid_argument("input filename must not be empty");
    std::ifstream input(filename, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open input file: " + filename);
    std::string line;
    if (!std::getline(input, line)) throw std::invalid_argument("input CSV is empty");
    if (line.size() >= 3 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
    if (trim(line) != "loss") {
        throw std::invalid_argument("CSV header must contain the single column loss");
    }
    std::vector<double> values;
    std::size_t line_number = 1;
    while (std::getline(input, line)) {
        ++line_number;
        if (values.size() == risk::max_observations) {
            throw std::invalid_argument("CSV contains more than 1000000 observations");
        }
        values.push_back(decimal(line, line_number));
    }
    if (input.bad()) throw std::runtime_error("failed to read input CSV");
    if (values.empty()) throw std::invalid_argument("CSV contains no loss observations");
    return values;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--help") {
            help();
            return 0;
        }
        std::map<std::string, std::string> options;
        for (int index = 1; index < argc; index += 2) {
            const std::string option(argv[index]);
            if (!known_option(option)) throw std::invalid_argument("unknown option: " + option);
            if (index + 1 == argc) throw std::invalid_argument("missing value for " + option);
            if (!options.emplace(option, argv[index + 1]).second) {
                throw std::invalid_argument("duplicate option: " + option);
            }
        }
        if (options.count("--input") == 0) {
            throw std::invalid_argument("--input is required");
        }
        const unsigned level = options.count("--confidence-bps") != 0
            ? confidence(options.at("--confidence-bps")) : 9500U;
        const std::string method = options.count("--method") != 0
            ? options.at("--method") : "partition";
        if (method != "sort" && method != "partition") {
            throw std::invalid_argument("--method must be sort or partition");
        }
        const auto losses = read_losses(options.at("--input"));
        const auto metrics = method == "sort" ? risk::estimate_sort(losses, level)
                                               : risk::estimate_partition(losses, level);
        std::cout.imbue(std::locale::classic());
        std::cout << std::setprecision(17)
                  << "metric,value\nmethod," << method
                  << "\nsample_count," << losses.size()
                  << "\nconfidence_bps," << level
                  << "\nvalue_at_risk," << metrics.value_at_risk
                  << "\nexpected_shortfall," << metrics.expected_shortfall
                  << "\nmean_loss," << metrics.mean_loss
                  << "\nworst_loss," << metrics.worst_loss << '\n';
        if (!std::cout) throw std::runtime_error("failed to write CSV output");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\nUse --help for usage.\n";
        return 1;
    }
}
