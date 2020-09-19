#include "echo_server_delegate.h"

void EchoResult::accumulate(Number number) {
    numbers.push_back(number);
    sum += number;
}

bool EchoResult::empty() const {
    return numbers.empty();
}

std::string EchoResult::toMessage() {
    if (empty()) {
        return "";
    }

    std::sort(numbers.begin(), numbers.end());

    std::stringstream ss;

    const size_t numNumbers = numbers.size();

    ss << numbers[0];

    for (size_t i = 1; i < numNumbers; ++i) {
        ss << " " << numbers[i];
    }

    ss << '\n';

    ss << sum;

    return ss.str();
}

std::string EchoServerDelegate::process(const std::string &message) noexcept {
    std::stringstream ss(message);

    Number tmp;

    EchoResult echoResult;

    while (!ss.eof()) {
        std::string tmpString;

        ss >> tmpString;

        if (std::stringstream(tmpString) >> tmp) {
            echoResult.accumulate(tmp);
        }
    }

    return echoResult.empty() ? message : echoResult.toMessage();
}