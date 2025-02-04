#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "TapeDev.hpp"
#include "TapeDevConfig.hpp"
#include "TapeDevExceptions.hpp"

TapeDev::TapeDev(const std::filesystem::path& t_tape_file_path, const TapeDevConfig& t_dev_config,
                 const TapeDevOperationMode t_mode) noexcept
    : m_tape_file_path(t_tape_file_path),
      m_dev_config(t_dev_config),
      m_operation_mode(t_mode),
      m_mem_buf(nullptr),
      m_mem_buf_index(0),
      m_head_pos(0),
      m_start_of_tape_flag(true),
      m_end_of_tape_flag(false),
      m_first_write_flag(false) {
  // Открываем файл устройства прямо в конструкторе. Не делаем
  // дополнительных проверок на успешность операции, потому что на стадии
  // проверки и обработки аргументов командной строки гарантируем валидный
  // путь.
  if (m_operation_mode == TapeDevOperationMode::Read) {
    m_tape_file.open(m_tape_file_path, std::ios::in);
  } else if (m_operation_mode == TapeDevOperationMode::Write) {
    m_tape_file.open(m_tape_file_path, std::ios::out | std::ios::trunc);
  } else if (m_operation_mode == TapeDevOperationMode::ReadWrite) {
    m_tape_file.open(m_tape_file_path, std::ios::in | std::ios::out);
  } else {  // TapeDevOperationMode::Append
    m_tape_file.open(m_tape_file_path, std::ios::out | std::ios::app);
  }

  if (m_operation_mode == TapeDevOperationMode::Write ||
      m_operation_mode == TapeDevOperationMode::Append) {
    m_first_write_flag = true;
  }

  m_mem_buf = new int[m_dev_config.mem_buf_size];
}

void TapeDev::doOneStepBackOnTape() noexcept {
  char ch;
  bool f = false;

  while (m_tape_file.get(ch)) {
    // двигаемся в _обратном_ направлении на один символ
    m_tape_file.seekg(-2, std::ios::cur);
    // m_tape_file.seekp(-2, std::ios::cur); ???

    if (m_tape_file.tellg() == 0 && m_tape_file.tellp() == 0) {
      // Для того, чтобы оказаться ровно на пробельном символе перед целевым
      // символом делаем один шаг вперёд.
      if (f && std::isspace(ch)) {
        m_tape_file.seekg(1, std::ios::cur);
      }
      break;
    }
    if (!f && std::isspace(ch)) {
      continue;
    }
    if (!f && std::isdigit(ch)) {
      f = true;
      continue;
    }
    if (f && std::isdigit(ch)) {
      continue;
    }
    if (f && std::isspace(ch)) {
      // Для того, чтобы оказаться ровно на пробельном символе перед целевым
      // символом делаем один шаг вперёд.
      m_tape_file.seekg(1, std::ios::cur);
      break;
    }
  }
}

int TapeDev::read() {
  if (m_operation_mode == TapeDevOperationMode::Read ||
      m_operation_mode == TapeDevOperationMode::ReadWrite) {
    if (!m_end_of_tape_flag) {
      int res = 0;

      std::string cell;
      char ch;

      while (m_tape_file.get(ch)) {
        if (std::isspace(ch)) {
          if (!cell.empty()) {
            // Возвращаем курсор чтения на один символ назад, чтобы
            // поддерживать состояние файла ленты, при котором операции
            // начинаются непосредственно на пробельном символе перед
            // целевым значением.
            m_tape_file.seekg(-1, std::ios::cur);

            // Делаем шаг назад. То есть приводим файл к состоянию, в котором
            // он был до осуществления чтения, потому что read сам по себе
            // не должен сдвигать ленту.
            doOneStepBackOnTape();

            break;
          }

          continue;
        }
        if (std::isdigit(ch)) {
          cell += ch;
        } else {
          throw BadTapeException("Недопустимый символ на ленте: '" + std::string(1, ch) + "'.");
        }
      }

      // Отмечаем в состоянии устройства достижение конца ленты.
      if (m_tape_file.eof()) {
        m_end_of_tape_flag = true;
      }

      // NOTE: оставлю этот блок с условием, потому что возможна ситуация,
      // при которой в конце файла будет оставлено несколько пробельных символов.
      // В этом случае код выше отработает корректно, но на данном этапе будет
      // получена пустая строка, что приведёт к ошибке преобразования.
      //
      // возможно, FIXME
      if (!cell.empty()) {
        // NOTE: std::stoi может выбросить std::invalid_argument и
        // std::out_of_range
        m_mem_buf[m_mem_buf_index] = std::stoi(cell);
        res = m_mem_buf[m_mem_buf_index];
        m_mem_buf_index = (m_mem_buf_index + 1) % m_dev_config.mem_buf_size;
      } else {
        throw BadTapeException();  // ???
      }

      // Возвращаем только что считанное в память значение как результат
      // операции чтения.
      return res;
    } else {
      // Если выполняется попытка чтения после достижения конца ленты, то
      // выбрасываем исключение.
      throw EndOfTapeException();
    }
  } else {
    throw InvalidOperationException(
        "Чтение невозможно. Устройство работает в режиме только запись.");
  }
  // Эмулируем время, необходимое устройству для выполнения чтения с ленты.
  std::this_thread::sleep_for(std::chrono::milliseconds(m_dev_config.read_delay));
}

void TapeDev::write(int t_value) {
  if (m_operation_mode == TapeDevOperationMode::Write ||
      m_operation_mode == TapeDevOperationMode::Append) {
    std::string val_str = std::to_string(t_value);
    if (!m_first_write_flag) {
      m_tape_file << " ";
    } else {
      m_first_write_flag = false;
    }
    m_tape_file << val_str;
    m_tape_file << std::flush;
  } else if (m_operation_mode == TapeDevOperationMode::ReadWrite) {
    // TODO: возможно, стоит здесь реализовать запись "по-честному",
    // используя swap-ленту.

    // Запоминаем текущую позицию курсора в файле.
    std::streampos pos = m_tape_file.tellp();

    // Для того, чтобы корректно перезаписать значения, нам нужно сохранить
    // "хвост" ленты.
    std::vector<int> swap;

    char ch;
    std::string curr_cell;

    // Считываем все значения справа от того, которое хотим перезаписать,
    // включая его самого.
    while (true) {
      curr_cell.clear();
      while (m_tape_file.get(ch)) {
        if (std::isspace(ch)) {
          if (!curr_cell.empty()) {
            break;
          }
          continue;
        }
        if (std::isdigit(ch)) {
          curr_cell += ch;
        } else {
          throw BadTapeException("Недопустимый символ на ленте: '" + std::string(1, ch) + "'.");
        }
      }
      if (!curr_cell.empty()) {
        // NOTE: std::stoi может выбросить std::invalid_argument и
        // std::out_of_range
        swap.push_back(std::stoi(curr_cell));
      } else {
        throw BadTapeException();  // ???
      }
      if (m_tape_file.eof()) {
        break;
      }
    }
    swap.at(0) = t_value;

    // нужно выполнить, чтобы корректно отработал seekp(...)
    m_tape_file.clear();

    // Возвращаем курсор в файле на исходную позицию.
    m_tape_file.seekp(pos, std::ios::beg);

    if (!m_start_of_tape_flag) {
      // Сдвигаем курсор на один символ вправо, чтобы начать перезаписывать
      // символы с целевого символа и не затереть пробел.
      m_tape_file.seekp(1, std::ios::cur);
    }

    for (size_t i = 0; i < swap.size(); ++i) {
      if (i == swap.size() - 1) {
        m_tape_file << swap.at(i);
      } else {
        m_tape_file << swap.at(i) << " ";
      }
    }

    m_tape_file.clear();
    // Возвращаем курсор файла в исходное положение.
    m_tape_file.seekp(pos);
  } else {
    throw InvalidOperationException(
        "Запись невозможна. Устройство работает в режиме только чтение.");
  }
  // Эмулируем время, необходимое устройству для выполнения записи на ленту.
  std::this_thread::sleep_for(std::chrono::milliseconds(m_dev_config.write_delay));
}

// TODO: учесть режим, в котором работает устройство. Если это Write или
// Append, то операции сдвига влево/вправо и перемотки в начало не будут
// иметь смысла.
void TapeDev::shiftLeft() {
  // Проверяем на то, что считывающая/записывающая магнитная головка уже
  // находится в начале ленты.
  if (m_start_of_tape_flag) {
    return;
  }

  char ch;
  bool f = false;

  // Если находились в конце ленты, то обновляем флаг m_end_of_tape_flag.
  if (m_end_of_tape_flag) {
    m_end_of_tape_flag = false;
  }

  while (m_tape_file.get(ch)) {
    // двигаемся в _обратном_ направлении на один символ
    m_tape_file.seekg(-2, std::ios::cur);
    // m_tape_file.seekp(-2, std::ios::cur); ???

    if (m_tape_file.tellg() == 0 && m_tape_file.tellp() == 0) {
      m_start_of_tape_flag = true;
      break;
    }
    if (!f && std::isspace(ch)) {
      continue;
    }
    if (!f && std::isdigit(ch)) {
      f = true;
      continue;
    }
    if (f && std::isdigit(ch)) {
      continue;
    }
    if (f && std::isspace(ch)) {
      // Для того, чтобы оказаться ровно на пробельном символе перед целевым
      // символом делаем один шаг вперёд.
      m_tape_file.seekg(1, std::ios::cur);
      break;
    }
  }
  if (m_head_pos != 0) {
    m_head_pos -= 1;
  }
  // Эмулируем время, необходимое устройству для выполнения сдвига на одну
  // позицию влево.
  std::this_thread::sleep_for(std::chrono::milliseconds(m_dev_config.shift_delay));
}

// TODO: учесть режим, в котором работает устройство. Если это Write или
// Append, то операции сдвига влево/вправо и перемотки в начало не будут
// иметь смысла.
void TapeDev::shiftRight() {
  // Проверяем на то, что считывающая/записывающая магнитная головка уже
  // находится в конце ленты.
  if (m_end_of_tape_flag) {
    return;
  }

  char ch;
  bool f = false;

  // Если находились в начале ленты, то обновляем флаг m_start_of_tape_flag.
  if (m_start_of_tape_flag) {
    m_start_of_tape_flag = false;
  }

  while (m_tape_file.get(ch)) {
    if (m_tape_file.eof()) {
      m_end_of_tape_flag = true;
      break;
    }
    if (!f && std::isspace(ch)) {
      continue;
    }
    if (!f && std::isdigit(ch)) {
      f = true;
      continue;
    }
    if (f && std::isdigit(ch)) {
      continue;
    }
    if (f && std::isspace(ch)) {
      break;
    }
  }

  // Поддерживаем состояние, при котором любая операция начинается на
  // пробельном символе непосредственно перед целевым значением.
  m_tape_file.seekg(-1, std::ios::cur);

  m_head_pos += 1;

  // Эмулируем время, необходимое устройству для выполнения сдвига на одну
  // позицию вправо.
  std::this_thread::sleep_for(std::chrono::milliseconds(m_dev_config.shift_delay));
}

// TODO: учесть режим, в котором работает устройство. Если это Write или
// Append, то операции сдвига влево/вправо и перемотки в начало не будут
// иметь смысла.
void TapeDev::rewind() {
  // Очищаем возможные предыдущие ошибки ввода/вывода.
  m_tape_file.clear();
  // Устанавливаем курсоры файла ленты в начало.
  m_tape_file.seekg(0, std::ios::beg);
  m_tape_file.seekp(0, std::ios::beg);
  // Отмечаем в состоянии объекта, что находимся в начале ленты.
  m_start_of_tape_flag = true;
  m_head_pos = 0;

  // Эмулируем время, необходимое устройству для выполнения перемотки ленты в
  // начало.
  std::this_thread::sleep_for(std::chrono::milliseconds(m_dev_config.rewind_delay));
}

size_t TapeDev::getHeadPos() const noexcept {
  return m_head_pos;
}

void TapeDev::replaceTape(const std::filesystem::path& t_new_tape_file_path,
                          TapeDevOperationMode t_mode) {
  m_tape_file_path = t_new_tape_file_path;
  m_operation_mode = t_mode;

  // Закрываем текущий файл ленты.
  m_tape_file.close();

  if (m_operation_mode == TapeDevOperationMode::Read) {
    m_tape_file.open(m_tape_file_path, std::ios::in);
  } else if (m_operation_mode == TapeDevOperationMode::Write) {
    m_tape_file.open(m_tape_file_path, std::ios::out | std::ios::trunc);
  } else if (m_operation_mode == TapeDevOperationMode::ReadWrite) {
    m_tape_file.open(m_tape_file_path, std::ios::in | std::ios::out);
  } else {  // TapeDevOperationMode::Append
    m_tape_file.open(m_tape_file_path, std::ios::out | std::ios::app);
  }

  if (m_operation_mode == TapeDevOperationMode::Write) {
    m_first_write_flag = true;
  }

  if (m_operation_mode == TapeDevOperationMode::Append && m_tape_file.tellp() == 0) {
    m_first_write_flag = true;
  }

  // Сбрасываем флаги состояния.
  m_tape_file.clear();
  m_head_pos = 0;
  if (m_operation_mode == TapeDevOperationMode::Append) {
    m_start_of_tape_flag = false;
    m_end_of_tape_flag = true;
  } else {
    m_start_of_tape_flag = true;
    m_end_of_tape_flag = false;
  }
}

bool TapeDev::atStartOfTape() const noexcept {
  return m_start_of_tape_flag;
}

bool TapeDev::atEndOfTape() const noexcept {
  return m_end_of_tape_flag;
}

int TapeDev::getMemBufCurrValue() const noexcept {
  return m_mem_buf[m_mem_buf_index];
}

int TapeDev::getMemBufValueAt(size_t t_index) const {
  if (t_index >= m_dev_config.mem_buf_size) {
    throw std::out_of_range("Переданный индекс выходит за границы буфера памяти.");
  }
  return m_mem_buf[t_index];
}

std::pair<std::vector<int>, size_t> TapeDev::getMemBufCopy() const noexcept {
  std::vector<int> copy(m_mem_buf, m_mem_buf + m_dev_config.mem_buf_size);
  return std::make_pair(copy, m_mem_buf_index);
}

void TapeDev::setMemBuf(const std::vector<int>& t_values) noexcept {
  size_t values_size = t_values.size();
  if (values_size > m_dev_config.mem_buf_size) {
    values_size = m_dev_config.mem_buf_size;
  }

  for (size_t i = 0; i < values_size; ++i) {
    m_mem_buf[i] = t_values.at(i);
  }

  // Если размер переданного массива меньше размера буфера памяти, то
  // оставшиеся ячейки заполняем нулями.
  for (size_t i = values_size; i < m_dev_config.mem_buf_size; ++i) {
    m_mem_buf[i] = 0;
  }

  // Сбрасываем индекс текущей позиции в буфере.
  m_mem_buf_index = 0;
}

void TapeDev::resetMemBufIndex() noexcept {
  m_mem_buf_index = 0;
}

size_t TapeDev::getDevMemBufSize() const noexcept {
  return m_dev_config.mem_buf_size;
}

TapeDev::~TapeDev() noexcept {
  m_tape_file.close();
  delete[] m_mem_buf;
}
