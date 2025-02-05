#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "TapeDevConfig.hpp"
#include "utils.hpp"

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
  std::string cfgOption;

  while (std::getline(input, cfgOption)) {
    // Если вдруг в начале/конце строки присутствуют пробельные символы,
    // удалим их сразу.
    int value = 0;
    try {
      if (cfgOption.empty() || stringStartsWith(cfgOption, "#")) {
        continue;
      } else if (stringStartsWith(cfgOption, "MemoryBufferSize:")) {
        value = std::stoi(trim_copy(splitAfterDelimiter(cfgOption)));
        cfg.mem_buf_size = value;
      } else if (stringStartsWith(cfgOption, "TapeReadDelay:")) {
        value = std::stoi(trim_copy(splitAfterDelimiter(cfgOption)));
        cfg.read_delay = value;
      } else if (stringStartsWith(cfgOption, "TapeWriteDelay:")) {
        value = std::stoi(trim_copy(splitAfterDelimiter(cfgOption)));
        cfg.write_delay = value;
      } else if (stringStartsWith(cfgOption, "TapeShiftDelay:")) {
        value = std::stoi(trim_copy(splitAfterDelimiter(cfgOption)));
        cfg.shift_delay = value;
      } else if (stringStartsWith(cfgOption, "TapeRewindDelay:")) {
        value = std::stoi(trim_copy(splitAfterDelimiter(cfgOption)));
        cfg.rewind_delay = value;
      } else {
        throw std::invalid_argument("Неизвестная опция в конфигурационном файле '" +
                                    t_cfgFilePath.string() + "'.");
      }
    } catch (const std::invalid_argument& e) {
      // NOTE: в этом месте можно добавить нечто вроде логирования (например,
      // просто выводом в терминал), чтобы объяснять, что именно не так в
      // конфигурационном файле.
      throw std::runtime_error("Недопустимое значение в конфигурационном файле '" +
                               t_cfgFilePath.string() + "'.");
    } catch (const std::out_of_range& e) {
      throw std::runtime_error(
          "Значение, указанное в конфигурационном файле, выходит за границы "
          "типа 'unsigned' '" +
          t_cfgFilePath.string() + "'.");
    }
  }

  return cfg;
}