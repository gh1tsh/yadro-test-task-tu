#include <cstdlib>
#include <filesystem>
#include <string>

struct TapeDevConf final {

  TapeDevConf() = default;

  TapeDevConf(const std::filesystem::path&, size_t, unsigned, unsigned,
              unsigned, unsigned);

  std::string to_string() const;

  // Путь к файлу с конфигурацией устройства.
  std::filesystem::path cfg_path;

  // Конфигурируемые параметры

  /// Размер буфера памяти устройства.
  size_t mem_buf_size;
  /// Значение задержки при чтении.
  unsigned read_delay;
  /// Значение задержки при записи.
  unsigned write_delay;
  /// Значение задержки при сдвиге на одну ячейку.
  unsigned shift_delay;
  /// Значение задержки при перемотке ленты в начало.
  unsigned rewind_delay;
};

const TapeDevConf parseTapeConfigFile(const std::filesystem::path&);