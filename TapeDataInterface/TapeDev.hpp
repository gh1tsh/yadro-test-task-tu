#ifndef TAPE_DEV_H
#define TAPE_DEV_H

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "ITapeDev.hpp"
#include "TapeDevConfig.hpp"

// FIXME: добавить документирующие комментарии.
/// Перечисление, определяющее возможные режимы работы ленточного устройства.
enum class TapeDevOperationMode { Read, Write, ReadWrite, Append };

class TapeDev final : public ITapeDev {
 public:
  TapeDev(const std::filesystem::path&, const TapeDevConfig&, const TapeDevOperationMode) noexcept;

  /// Пытается считать значение из текущей ячейки ленты и записывает его в
  /// текущую позицию в буфере памяти, на которое указывает m_mem_buf_index.
  ///
  /// Всегда возвращает считанное значение. При возникновении ошибок или при
  /// попытке чтения из конца ленты выбрасывает исключения.
  ///
  /// При достижении конца ленты во время чтения, устанавливает
  /// m_end_of_tape_flag.
  int read() override;

  // FIXME: добавить документирующие комментарии.
  void write(int) override;

  // FIXME: добавить документирующие комментарии.
  void shiftLeft() override;

  // FIXME: добавить документирующие комментарии.
  void shiftRight() override;

  // FIXME: добавить документирующие комментарии.
  void rewind() override;

  // FIXME: добавить документирующие комментарии.
  size_t getHeadPos() const noexcept;

  // FIXME: добавить документирующие комментарии.
  void replaceTape(const std::filesystem::path&, TapeDevOperationMode);

  bool atStartOfTape() const noexcept;

  bool atEndOfTape() const noexcept;

  /// Возвращает элемент буфера памяти, на который в данный момент указывает
  /// индекс буфера.
  int getMemBufCurrValue() const noexcept;

  /// Возвращает элемент буфера памяти, находящийся на позиции, переданной
  /// в качестве аргумента.
  ///
  /// В случае, если переданный индекс выходит за границы буфера памяти,
  /// выбрасывает исключение std::out_of_range.
  int getMemBufValueAt(size_t) const;

  /// Возвращает пару: копию буфера памяти устройства в текущем состоянии и
  /// индекс текущей позиции в буфере.
  std::pair<std::vector<int>, size_t> getMemBufCopy() const noexcept;

  /// Загружает в оперативную память ленточного устройства переданный массив
  /// значений.
  ///
  /// Если размер переданного массива больше размера оперативной памяти
  /// устройства, то загрузит только M первых значений (M - размер буфера
  /// памяти устройства).
  ///
  /// Если размер переданного массива меньше размера буфера памяти, то в
  /// начало буфера памяти запишет переданные значения, а оставшиеся ячейки
  /// заполнит нулями.
  void setMemBuf(const std::vector<int>&) noexcept;

  // FIXME: добавить документирующие коментарии.
  void resetMemBufIndex() noexcept;

  size_t getDevMemBufSize() const noexcept;

  ~TapeDev() noexcept;

 private:
  /// Выполняет сдвиг считывающей/записывающей магнитной головки на одно
  /// значение назад (влево) на ленте. Функция нужна для метода read(), в
  /// котором использование shiftLeft() вызывает неправильное поведение и
  /// переполнение m_head_pos.
  void doOneStepBackOnTape() noexcept;

  /// Путь к файлу ленты.
  std::filesystem::path m_tape_file_path;

  /// Объект, который хранит конфигурацию устройства.
  const TapeDevConfig m_dev_config;

  /// Режим работы ленточного устройства.
  TapeDevOperationMode m_operation_mode;

  /// Буфер памяти устройства.
  ///
  /// Если будет заполнен полсностью, новые значения будут перезаписывать
  /// старые из начала.
  int* m_mem_buf;

  /// Текущая позиция чтения/записи в буфере памяти устройства.
  size_t m_mem_buf_index;

  /// Текущая позиция считывающей/записыващей магнитной головки на ленте.
  size_t m_head_pos;

  /// Флаг, указывающий на то, что считывающая/записывающая магнитная головка
  /// находится в начале ленты.
  bool m_start_of_tape_flag;

  /// Флаг, указывающий на то, что считывающая/записывающая магнитная головка
  /// находится в конце ленты.
  bool m_end_of_tape_flag;

  /// Флаг, показывающий совершение самой первой операции записи в файл. Нужен
  /// для того, чтобы записать только значение и не записывать предшествующий
  /// пробел. Этот флаг будет использоваться только в режимах
  /// TapeDevOperationMode::Write и TapeDevOperationMode::Append.
  bool m_first_write_flag;

  /// Файл ленты.
  std::fstream m_tape_file;
};

#endif  // TAPE_DEV_H
