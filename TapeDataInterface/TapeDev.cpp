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
      if (!cell.empty()) {
        try {
          m_mem_buf[m_mem_buf_index] = std::stoi(cell);
        } catch (const std::exception& e) {
          throw BadTapeException(
              "Не удалось выполнить преобразование значения с ленты в целое цисло. Значение: " +
              cell + ".");
        }
        res = m_mem_buf[m_mem_buf_index];
        m_mem_buf_index = (m_mem_buf_index + 1) % m_dev_config.mem_buf_size;
      } else {
        throw BadTapeException(
            "Не удалось считать значение с ленты. Возможно, в конце файла ленты присутствуют "
            "лишние пробелы.");
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
    std::string target_value = std::to_string(t_value);

    std::string swap_tape_file_name = "swap_tape.txt";
    std::string target_tape_file_name = m_tape_file_path.filename().string();
    std::filesystem::path target_tape_file_dir = m_tape_file_path.parent_path();
    std::filesystem::path swap_tape_file_path = target_tape_file_dir / swap_tape_file_name;

    // Запоминаем текущую позицию курсора в файле.
    std::streamoff pos = std::streamoff(m_tape_file.tellp());
    if (pos != 0) {
      pos += 1;
    }

    // Открываем файл "swap_tape.txt" для записи.
    std::ofstream swap_tape_file(swap_tape_file_path, std::ios::out | std::ios::trunc);

    char ch;
    std::string cell;

    m_tape_file.seekp(0, std::ios::beg);

    while (true) {
      cell.clear();
      while (m_tape_file.get(ch)) {
        if (std::isspace(ch)) {
          if (!cell.empty()) {
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

      if (!cell.empty()) {
        try {
          int res = std::stoi(cell);

          if (swap_tape_file.tellp() == pos) {
            swap_tape_file << target_value;
          } else {
            swap_tape_file << res;
          }
          swap_tape_file << std::flush;
        } catch (const std::exception& e) {
          throw BadTapeException(
              "Не удалось выполнить преобразование значения с ленты в целое цисло. Значение: " +
              cell + ".");
        }
      } else {
        throw BadTapeException(
            "Не удалось считать значение с ленты. Возможно, в конце файла ленты присутствуют "
            "лишние пробелы.");
      }

      if (m_tape_file.eof()) {
        break;
      } else {
        swap_tape_file << " ";
      }
    }

    m_tape_file.close();

    // Удаляем текущий файл ленты.
    std::filesystem::remove(m_tape_file_path);

    // Переименовываем swap-файл в текущий файл ленты.
    std::filesystem::rename(swap_tape_file_path, m_tape_file_path);

    // Открываем сформированный файл ленты.
    m_tape_file.open(m_tape_file_path, std::ios::in | std::ios::out);

    m_tape_file.seekp(pos, std::ios::beg);
  } else {
    throw InvalidOperationException(
        "Запись невозможна. Устройство работает в режиме только чтение.");
  }
  // Эмулируем время, необходимое устройству для выполнения записи на ленту.
  std::this_thread::sleep_for(std::chrono::milliseconds(m_dev_config.write_delay));
}

void TapeDev::shiftLeft() {
  if (m_operation_mode == TapeDevOperationMode::Write) {
    throw InvalidOperationException(
        "В режиме работы устройств TapeDevOperationMode::Write сдвиг влево не поддерживается.");
  }
  if (m_operation_mode == TapeDevOperationMode::Append) {
    throw InvalidOperationException(
        "В режиме работы устройств TapeDevOperationMode::Append сдвиг влево не поддерживается.");
  }

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

void TapeDev::shiftRight() {
  if (m_operation_mode == TapeDevOperationMode::Write) {
    throw InvalidOperationException(
        "В режиме работы устройств TapeDevOperationMode::Write сдвиг право не поддерживается.");
  }
  if (m_operation_mode == TapeDevOperationMode::Append) {
    throw InvalidOperationException(
        "В режиме работы устройств TapeDevOperationMode::Append сдвиг вправо не поддерживается.");
  }

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

  if (m_tape_file.eof()) {
    m_end_of_tape_flag = true;
  } else {
    // Поддерживаем состояние, при котором любая операция начинается на
    // пробельном символе непосредственно перед целевым значением.
    m_tape_file.seekg(-1, std::ios::cur);
  }

  m_head_pos += 1;

  // Эмулируем время, необходимое устройству для выполнения сдвига на одну
  // позицию вправо.
  std::this_thread::sleep_for(std::chrono::milliseconds(m_dev_config.shift_delay));
}

void TapeDev::rewind() {
  if (m_operation_mode == TapeDevOperationMode::Write) {
    throw InvalidOperationException(
        "В режиме работы устройств TapeDevOperationMode::Write перемотка ленты в начало не "
        "поддерживается.");
  }
  if (m_operation_mode == TapeDevOperationMode::Append) {
    throw InvalidOperationException(
        "В режиме работы устройств TapeDevOperationMode::Append перемотка ленты в начало не "
        "поддерживается.");
  }

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

  if (!m_tape_file.is_open()) {
    throw BadTapeException("Не удалось отктыть файл ленты '" + m_tape_file_path.string() + "'.");
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
