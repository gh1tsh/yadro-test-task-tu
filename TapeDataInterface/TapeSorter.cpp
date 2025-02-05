#include <algorithm>
#include <exception>
#include <iostream>  // DEBUG
#include <limits>
#include <vector>

#include "TapeSorter.hpp"

TapeSorter::TapeSorter(TapeDev& t_tape_dev, const std::filesystem::path& t_target_tape_file_path,
                       const std::filesystem::path& t_output_tape_file_path,
                       const std::filesystem::path& t_working_dir_path) noexcept
    : m_tape_dev(t_tape_dev),
      m_target_tape_file_path(t_target_tape_file_path),
      m_output_tape_file_path(t_output_tape_file_path),
      m_working_dir_path(t_working_dir_path),
      m_shortcut_flag(false),
      m_num_values_on_temp_tapes(),
      m_temp_tapes_counter(0),
      m_values_counter(0) {}

void TapeSorter::sort() {
  setup();

  // DEBUG CODE
  std::cout << "m_shortcut_flag: " << m_shortcut_flag << std::endl;
  debugPrintMemBuf();
  std::cout << std::endl;
  // END OF DEBUG CODE

  if (m_shortcut_flag) {
    // Копия буфера памяти устройства для сортировки со всеми значениями с
    // входной ленты.
    std::vector<int> buf_to_sort = m_tape_dev.getMemBufCopy().first;

    // Сортируем вектор значений стандартным std::sort(...).
    std::sort(buf_to_sort.begin(), buf_to_sort.end());

    // Пишем отсортированные значения на выходную ленту и завершаем сортировку.
    m_tape_dev.replaceTape(m_output_tape_file_path, TapeDevOperationMode::Write);
    for (size_t i = 0; i < buf_to_sort.size(); ++i) {
      try {
        m_tape_dev.write(buf_to_sort.at(i));
      } catch (const std::exception& e) {
        // TODO: реализовать обработку исключений.
      }
    }
  } else {
    forward_pass();
    backward_pass();
  }
}

void TapeSorter::debugPrintMemBuf() const noexcept {
  std::cout << std::endl;
  std::cout << "Memory buffer: ";
  for (size_t i = 0; i < m_tape_dev.getDevMemBufSize(); ++i) {
    std::cout << m_tape_dev.getMemBufValueAt(i) << " ";
  }
  std::cout << std::endl;
}

void TapeSorter::setup() {
  m_tape_dev.resetMemBufIndex();

  debugPrintMemBuf();  // DEBUG

  // Количество значений, которое было считано с входной ленты в данной
  // итерации.
  size_t num_read_values = 0;

  // Так как в процессе подготовки мы меняем ленты, то состояние входной ленты
  // теряется. Достигли мы конца входной ленты или нет, будем хранить в
  // отдельном флаге.
  bool end_of_input_tape_flag = false;

  // Делаем попытку прочитать все значения с ленты в буфер памяти.
  for (size_t i = 0; i < m_tape_dev.getDevMemBufSize(); ++i) {
    try {
      m_tape_dev.read();
      num_read_values += 1;
      m_tape_dev.shiftRight();
    } catch (const std::exception& e) {
      // TODO: реализовать обработку всех исключений, которые могут быть
      // выброшены из метода read().
    }

    if (m_tape_dev.atEndOfTape() && i <= m_tape_dev.getDevMemBufSize()) {
      m_shortcut_flag = true;
      break;
    }

    debugPrintMemBuf();  // DEBUG
  }

  // Если все значения из входной ленты сразу не поместились в память
  // устройства, то осуществляем подготовку временных лент.
  if (!m_shortcut_flag) {
    size_t input_tape_curr_head_pos = 0;

    while (true) {
      input_tape_curr_head_pos = m_tape_dev.getHeadPos();

      m_num_values_on_temp_tapes.push_back(num_read_values);

      m_values_counter += num_read_values;

      debugPrintMemBuf();  // DEBUG

      try {
        make_temp_tape();
      } catch (const std::exception& e) {
        // TODO: реализовать обработку исключений.
      }

      // Так как мы уже заполнили буфер памяти новыми значениями, то сначала
      // выгрузим их на временну ленту.
      std::vector<int> buf_to_write = m_tape_dev.getMemBufCopy().first;
      m_tape_dev.replaceTape(m_temp_tape_file_paths.at(m_temp_tapes_counter - 1),
                             TapeDevOperationMode::Write);
      for (size_t i = 0; i < num_read_values; ++i) {
        try {
          m_tape_dev.write(buf_to_write.at(i));
        } catch (const std::exception& e) {
          // TODO: реализовать обработку исключений.
        }
      }

      // На данном этапе последние считанные значения записаны на временную
      // ленту, поэтому просто выходим из цикла.
      if (end_of_input_tape_flag) {
        break;
      }

      // Готовимся считывать новую порцию значений с входной ленты.
      num_read_values = 0;

      m_tape_dev.replaceTape(m_target_tape_file_path, TapeDevOperationMode::Read);

      // Перемещаем считывающую/записывающую магнитную головку в позицию,
      // на которой остановились.
      for (int i = 0; i < input_tape_curr_head_pos; ++i) {
        m_tape_dev.shiftRight();
      }

      // Считываем в память новую порцию значений с входной ленты.
      for (size_t i = 0; i < m_tape_dev.getDevMemBufSize(); ++i) {
        try {
          m_tape_dev.read();
          num_read_values += 1;
          m_tape_dev.shiftRight();
        } catch (const std::exception& e) {
          // TODO: реализовать обработку всех исключений, которые могут быть
          // выброшены из метода read().
        }
        if (m_tape_dev.atEndOfTape()) {
          end_of_input_tape_flag = true;
          break;
        }
      }
    }
  }
}

void TapeSorter::forward_pass() {
  for (size_t i = 0; i < m_temp_tape_file_paths.size(); ++i) {
    m_tape_dev.replaceTape(m_temp_tape_file_paths.at(i), TapeDevOperationMode::Read);

    m_tape_dev.resetMemBufIndex();

    for (size_t i = 0; i < m_tape_dev.getDevMemBufSize(); ++i) {
      try {
        m_tape_dev.read();
        m_tape_dev.shiftRight();
      } catch (const std::exception& e) {
        // TODO: реализовать обработку исключений.
      }
      if (m_tape_dev.atEndOfTape()) {
        break;
      }
    }

    std::vector<int> buf_to_sort = m_tape_dev.getMemBufCopy().first;

    // Удаляем ненужные для нас значения, которые находятся в буфере.
    if (buf_to_sort.size() > m_num_values_on_temp_tapes.at(i)) {
      buf_to_sort.erase(buf_to_sort.begin() + m_num_values_on_temp_tapes.at(i), buf_to_sort.end());
    }

    std::sort(buf_to_sort.begin(), buf_to_sort.end());

    m_tape_dev.replaceTape(m_temp_tape_file_paths.at(i), TapeDevOperationMode::Write);

    for (size_t i = 0; i < buf_to_sort.size(); ++i) {
      try {
        m_tape_dev.write(buf_to_sort.at(i));
      } catch (const std::exception& e) {
        // TODO: реализовать обработку исключений.
      }
    }
  }
}

void TapeSorter::backward_pass() {
  std::vector<int> temp_tapes_indices(m_temp_tapes_counter);
  size_t temp_tape_idx = 0;
  int min_val = std::numeric_limits<int>::max();
  int curr_val = 0;
  // Количество значений, которое нужно обработать.
  int t = m_values_counter;

  while (t > 0) {
    for (size_t i = 0; i < temp_tapes_indices.size(); ++i) {
      // Если все значения на данной временной ленте обработаны, то пропускаем
      // её и приступаем к следующей.
      if (temp_tapes_indices.at(i) == m_num_values_on_temp_tapes.at(i)) {
        continue;
      }

      m_tape_dev.replaceTape(m_temp_tape_file_paths.at(i), TapeDevOperationMode::Read);

      for (size_t j = 0; j < temp_tapes_indices.at(i) && j < m_num_values_on_temp_tapes.at(i);
           ++j) {
        m_tape_dev.shiftRight();
      }

      if (m_tape_dev.atEndOfTape()) {
        continue;
      }

      try {
        curr_val = m_tape_dev.read();
      } catch (const std::exception& e) {
        // TODO: реализовать обработку исключений.
      }

      if (curr_val < min_val) {
        min_val = curr_val;
        temp_tape_idx = i;
      }
    }

    m_tape_dev.replaceTape(m_output_tape_file_path, TapeDevOperationMode::Append);

    try {
      m_tape_dev.write(min_val);
    } catch (const std::exception& e) {
      // TODO: реализовать обработку исключений.
    }
    t -= 1;
    temp_tapes_indices.at(temp_tape_idx) += 1;
    min_val = std::numeric_limits<int>::max();
  }
}

void TapeSorter::make_temp_tape() {
  std::filesystem::path new_temp_tape_file_path = m_working_dir_path;

  new_temp_tape_file_path.append("var").append("tmp").append(
      "temp_tape_" + std::to_string(m_temp_tapes_counter) + ".txt");

  m_temp_tape_file_paths.push_back(new_temp_tape_file_path);

  m_temp_tapes_counter += 1;

  std::fstream temp_tape_file(new_temp_tape_file_path, std::ios::out | std::ios::trunc);

  if (!temp_tape_file.is_open()) {
    throw std::runtime_error("не удалось создать файл временной ленты '" +
                             new_temp_tape_file_path.string() + "'.");
  }

  temp_tape_file.close();
}

TapeSorter::~TapeSorter() {}