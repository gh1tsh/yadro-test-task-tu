#ifndef TAPE_DEV_EXCEPTIONS
#define TAPE_DEV_EXCEPTIONS

#include <exception>
#include <stdexcept>

class EndOfTapeException : public std::exception {
 public:
  const char* what() const noexcept override { return "Достигнут конец ленты."; }
};

class BadTapeException : public std::runtime_error {
 public:
  BadTapeException()
      : std::runtime_error(
            "Файл ленты не является валидным или на ленте обнаружено "
            "недопустимое значение.") {}

  BadTapeException(const std::string& msg) : std::runtime_error(msg) {}

  const char* what() const noexcept override { return std::runtime_error::what(); }
};

class InvalidOperationException : public std::runtime_error {
 public:
  InvalidOperationException()
      : std::runtime_error("Операция не соответствует режиму работы устройства.") {}

  InvalidOperationException(const std::string& msg) : std::runtime_error(msg) {}

  const char* what() const noexcept override { return std::runtime_error::what(); }
};

#endif  // TAPE_DEV_EXCEPTIONS