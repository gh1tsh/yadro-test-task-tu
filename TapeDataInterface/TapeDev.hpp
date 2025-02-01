#ifndef TAPE_DEV_H
#define TAPE_DEV_H

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "ITapeDev.hpp"
#include "TapeDevConf.hpp"

/// Перечисление, определяющее возможные режимы работы ленточного устройства.
enum class TapeDevOperationMode { Read, Write, ReadWrite };

class TapeDev : public ITapeDev {
 public:
  TapeDev(const std::filesystem::path&, const TapeDevConf,
          const TapeDevOperationMode) noexcept;

  /// Пытается считать значение из текущей ячейки ленты и записывает его в
  /// текущую позицию в буфере памяти, на которое указывает m_mem_buf_index.
  ///
  /// Всегда возвращает считанное значение. При возникновении ошибок или при
  /// попытке чтения из конца ленты выбрасывает исключения.
  ///
  /// При достижении конца ленты во время чтения, устанавливает
  /// m_end_of_tape_flag.
  int read() override;

  void write(int) override;

  void shiftLeft() override;

  void shiftRight() override;

  void rewind() override;

  /// Возвращает элемент буфера памяти, на который в данный момент указывает
  /// индекс буфера.
  int getMemBufCurrValue() const noexcept;

  /// Возвращает пару: ссылку на константный вектор, содержащий значения
  /// буфера памяти в текущем состоянии, и индекс текущей позиции в буфере.
  // std::pair<const std::vector<int>&, size_t> getMemBufRef() const noexcept;

  /// Возвращает пару: копию буфера памяти устройства в текущем состоянии и
  /// индекс текущей позиции в буфере.
  std::pair<std::vector<int>, size_t> getMemBufCopy() const noexcept;

  ~TapeDev() noexcept;

 private:
  /// Путь к файлу ленты.
  const std::filesystem::path m_tape_file_path;

  /// Объект, который хранит конфигурацию устройства.
  const TapeDevConf m_dev_conf;

  /// Режим работы ленточного устройства.
  const TapeDevOperationMode m_operation_mode;

  /// Файл ленты.
  std::fstream m_tape_file;

  // В данный момент по моей задумке буфер памяти будет кольцевым, то есть
  // при достижении конца буфера, указатель будет перемещаться в начало
  // буфера и новые значения будут перезаписывать старые.

  /// Буфер памяти устройства.
  int* m_mem_buf;

  // Так как класс std::fstream в своём состоянии хранит позицию курсора в
  // в файле, то данное поле будет использоваться для запоминания текущей
  // позиции в буфере.

  /// Текущая позиция чтения/записи в буфере памяти устройства.
  size_t m_mem_buf_index;

  /// Флаг, указывающий на то, что считывающая/записывающая магнитная головка
  /// находится в начале ленты.
  bool m_tape_begin_flag;

  /// Флаг, указывающий на то, что считывающая/записывающая магнитная головка
  /// находится в конце ленты.
  bool m_end_of_tape_flag;
};

#endif  // TAPE_DEV_H
