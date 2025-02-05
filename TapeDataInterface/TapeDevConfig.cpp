#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "TapeDevConfig.hpp"
#include "utils.hpp"

TapeDevConfig::TapeDevConfig()
    : mem_buf_size(0), read_delay(0), write_delay(0), shift_delay(0), rewind_delay(0) {}

TapeDevConfig::TapeDevConfig(const std::filesystem::path& t_cfg_path, size_t t_mem_buf_size,
                             int t_read_delay, int t_write_delay, int t_shift_delay,
                             int t_rewind_delay)
    : cfg_path(t_cfg_path),
      mem_buf_size(t_mem_buf_size),
      read_delay(t_read_delay),
      write_delay(t_write_delay),
      shift_delay(t_shift_delay),
      rewind_delay(t_rewind_delay) {}

std::string TapeDevConfig::to_string() const {
  return "MemoryBufferSize: " + std::to_string(mem_buf_size) +
         "\nTapeReadDelay: " + std::to_string(read_delay) +
         "\nTapeWriteDelay: " + std::to_string(write_delay) +
         "\nTapeShiftDelay: " + std::to_string(shift_delay) +
         "\nTapeRewindDelay: " + std::to_string(rewind_delay);
}

const TapeDevConfig parseTapeConfigFile(const std::filesystem::path& t_cfgFilePath) {

  std::ifstream input(t_cfgFilePath);

  if (!input.is_open()) {
    throw std::runtime_error("Не удалось открыть для чтения файл '" + t_cfgFilePath.string() +
                             "'.");
  }

  TapeDevConfig cfg;
  std::string cfg_line;

  while (std::getline(input, cfg_line)) {
    // Если вдруг в начале/конце строки присутствуют пробельные символы,
    // удалим их сразу.
    int value = 0;
    try {
      if (cfg_line.empty() || stringStartsWith(cfg_line, "#")) {
        continue;
      } else if (stringStartsWith(cfg_line, "MemoryBufferSize:")) {
        value = std::stoi(trim_copy(splitAfterDelimiter(cfg_line)));
        if (value <= 1) {
          throw std::runtime_error("Значение 'MemoryBufferSize' должно быть больше 1.");
        }
        cfg.mem_buf_size = value;
      } else if (stringStartsWith(cfg_line, "TapeReadDelay:")) {
        value = std::stoi(trim_copy(splitAfterDelimiter(cfg_line)));
        cfg.read_delay = value;
      } else if (stringStartsWith(cfg_line, "TapeWriteDelay:")) {
        value = std::stoi(trim_copy(splitAfterDelimiter(cfg_line)));
        cfg.write_delay = value;
      } else if (stringStartsWith(cfg_line, "TapeShiftDelay:")) {
        value = std::stoi(trim_copy(splitAfterDelimiter(cfg_line)));
        cfg.shift_delay = value;
      } else if (stringStartsWith(cfg_line, "TapeRewindDelay:")) {
        value = std::stoi(trim_copy(splitAfterDelimiter(cfg_line)));
        cfg.rewind_delay = value;
      } else {
        throw std::runtime_error("Неизвестная опция в конфигурационном файле '" +
                                 t_cfgFilePath.string() + "': " + cfg_line + ".");
      }
    } catch (const std::invalid_argument& e) {
      throw std::runtime_error("Недопустимое значение в конфигурационном файле '" +
                               t_cfgFilePath.string() + "': " + cfg_line + ".");
    } catch (const std::out_of_range& e) {
      throw std::runtime_error(
          "Значение, указанное в конфигурационном файле, выходит за границы  типа 'int'.");
    }
  }

  return cfg;
}