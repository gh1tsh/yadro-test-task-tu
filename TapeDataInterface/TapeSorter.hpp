#ifndef TAPE_SORTER_HPP
#define TAPE_SORTER_HPP

#include <filesystem>
#include <vector>

#include "TapeDev.hpp"

class TapeSorter final {
 public:
  TapeSorter(TapeDev&, const std::filesystem::path&, const std::filesystem::path&,
             const std::filesystem::path&) noexcept;

  // FIXME: добавить документирующие комментарии.
  void sort();

  ~TapeSorter();

 private:
  // FIXME: добавить документирующие комментарии.
  void setup();

  // FIXME: добавить документирующие комментарии.
  void forward_pass();

  // FIXME: добавить документирующие комментарии.
  void backward_pass();

  // FIXME: добавить документирующие комментарии.
  void makeTempTape();

  // FIXME: добавить документирующие комментарии.
  TapeDev& m_tape_dev;

  // FIXME: добавить документирующие комментарии.
  const std::filesystem::path m_target_tape_file_path;

  // FIXME: добавить документирующие комментарии.
  const std::filesystem::path m_output_tape_file_path;

  // FIXME: добавить документирующие комментарии.
  const std::filesystem::path m_working_dir_path;

  // FIXME: добавить документирующие комментарии.
  std::vector<std::string> m_temp_tape_file_paths;

  /// Показывает, что на стадии setup все элементы входной ленты получилось
  /// прочитать в память устройства. Следовательно, можно сразу произвести
  /// их соритровку и запись на выходную ленту.
  bool m_shortcut_flag;

  /// Вектор, который хранит количество значений, содержащихся на каждой временной
  /// ленте после выполнения TapeSorter::setup().
  std::vector<int> m_num_values_on_temp_tapes;

  // FIXME: добавить документирующие комментарии.
  size_t m_temp_tapes_counter;

  // FIXME: добавить документирующие комментарии.
  size_t m_values_counter;
};

#endif  // TAPE_SORTER_HPP