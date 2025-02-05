#ifndef TAPE_DEV_CONF
#define TAPE_DEV_CONF

#include <cstdlib>
#include <filesystem>
#include <string>

// TODO: добавить проверку на то, что в конфигурацию передан ненулевой размер
// буфера памяти устройства.

struct TapeDevConfig final {

  TapeDevConfig();

  TapeDevConfig(const std::filesystem::path&, size_t, int, int, int, int);

  TapeDevConfig(const TapeDevConfig&) = default;

  TapeDevConfig& operator=(const TapeDevConfig&) = default;

  std::string to_string() const;

  // Путь к файлу с конфигурацией устройства.
  std::filesystem::path cfg_path;

  // Конфигурируемые параметры

  /// Размер буфера памяти устройства.
  size_t mem_buf_size;
  /// Значение задержки при чтении.
  int read_delay;
  /// Значение задержки при записи.
  int write_delay;
  /// Значение задержки при сдвиге на одну ячейку.
  int shift_delay;
  /// Значение задержки при перемотке ленты в начало.
  int rewind_delay;
};

const TapeDevConfig parseTapeConfigFile(const std::filesystem::path&);

#endif  // TAPE_DEV_CONF