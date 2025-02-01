#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "TapeDev.hpp"
#include "TapeDevExceptions.hpp"

TapeDev::TapeDev(const std::filesystem::path& t_tape_file_path,
                 const TapeDevConf t_dev_conf,
                 const TapeDevOperationMode t_mode) noexcept
    : m_tape_file_path(t_tape_file_path),
      m_dev_conf(t_dev_conf),
      m_operation_mode(t_mode),
      m_mem_buf_index(0),
      m_tape_begin_flag(true),
      m_end_of_tape_flag(false) {
  // Открываем файл устройства прямо в конструкторе. Не делаем
  // дополнительных проверок на успешность операции, потому что на стадии
  // проверки и обработки аргументов командной строки гарантируем валидный
  // путь.
  if (m_operation_mode == TapeDevOperationMode::Read) {
    m_tape_file.open(m_tape_file_path, std::ios::in);
  } else if (m_operation_mode == TapeDevOperationMode::Write) {
    m_tape_file.open(m_tape_file_path, std::ios::out);
  } else {
    m_tape_file.open(m_tape_file_path, std::ios::in | std::ios::out);
  }

  m_mem_buf = new int[m_dev_conf.mem_buf_size];
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
            // Возвращаем курсоры чтения и записи на один символ назад, чтобы
            // поддерживать состояние файла ленты, при котором операции
            // чтения/записи начинаются непосредственно на пробельном символе
            // перед целевым значением.
            m_tape_file.seekg(-1, std::ios::cur);
            m_tape_file.seekp(-1, std::ios::cur);
            this->shiftLeft();
            break;
          }
          continue;
        }
        if (std::isdigit(ch)) {
          cell += ch;
        } else {
          throw BadTapeException("Недопустимый символ на ленте: '" +
                                 std::string(1, ch) + "'.");
        }
      }

      // Отмечаем в состоянии устройства достижение конца ленты.
      if (m_tape_file.eof()) {
        m_end_of_tape_flag = true;
      }

      // NOTE: оставлю этот блок с условием, потому что возможна ситуация, при
      // которой в конце файла будет оставлено несколько пробельных символов. В
      // этом случае код выше отработает корректно, но на данном этапе будет
      // получена пустая строка, что приведёт к ошибке преобразования.

      // возможно, FIXME
      if (!cell.empty()) {
        // NOTE: std::stoi может выбросить std::invalid_argument и
        // std::out_of_range
        m_mem_buf[m_mem_buf_index] = std::stoi(cell);
        res = m_mem_buf[m_mem_buf_index];
        m_mem_buf_index = (m_mem_buf_index + 1) % m_dev_conf.mem_buf_size;
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
  std::this_thread::sleep_for(std::chrono::milliseconds(m_dev_conf.read_delay));
}

void TapeDev::write(int t_value) {
  if (m_operation_mode == TapeDevOperationMode::Write) {
    std::string val_str = std::to_string(t_value);
    // TODO: нужно учесть ситуацию записи последнего значения в файл. В этом
    // случае не нужно добавлять пробел в конце строки.
    m_tape_file << val_str << " ";
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
          throw BadTapeException("Недопустимый символ на ленте: '" +
                                 std::string(1, ch) + "'.");
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

    // Сдвигаем курсор на один символ вправо, чтобы начать перезаписывать
    // символы с целевого символа и не затереть пробел.
    m_tape_file.seekp(1, std::ios::cur);

    for (size_t i = 0; i < swap.size(); ++i) {
      if (i == swap.size() - 1) {
        m_tape_file << swap.at(i);
      } else {
        m_tape_file << swap.at(i) << " ";
      }
    }

    m_tape_file.clear();
    m_tape_file.seekp(pos);
  } else {
    throw InvalidOperationException(
        "Запись невозможна. Устройство работает в режиме только чтение.");
  }
  // Эмулируем время, необходимое устройству для выполнения записи на ленту.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(m_dev_conf.write_delay));
}

void TapeDev::shiftLeft() {
  // Проверяем на то, что считывающая/записывающая магнитная головка уже
  // находится в начале ленты.
  if (m_tape_begin_flag) {
    return;
  }

  char ch;
  bool f = false;

  while (m_tape_file.get(ch)) {
    if (m_tape_file.tellg() == 0) {
      m_tape_begin_flag = true;
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
    // двигаемся в _обратном_ направлении на один символ
    m_tape_file.seekg(-2, std::ios::cur);
    m_tape_file.seekp(-2, std::ios::cur);
  }
  // Эмулируем время, необходимое устройству для выполнения сдвига на одну
  // позицию влево.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(m_dev_conf.shift_delay));
}

void TapeDev::shiftRight() {
  // Проверяем на то, что считывающая/записывающая магнитная головка уже
  // находится в конце ленты/.
  if (m_end_of_tape_flag) {
    return;
  }

  char ch;
  bool f = false;

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
  // Эмулируем время, необходимое устройству для выполнения сдвига на одну
  // позицию вправо.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(m_dev_conf.shift_delay));
}

void TapeDev::rewind() {
  // Очищаем возможные предыдущие ошибки ввода/вывода.
  m_tape_file.clear();
  // Устанавливаем курсоры файла ленты в начало.
  m_tape_file.seekg(0, std::ios::beg);
  m_tape_file.seekp(0, std::ios::beg);
  // Отмечаем в состоянии объекта, что находимся в начале ленты.
  m_tape_begin_flag = true;

  // Эмулируем время, необходимое устройству для выполнения перемотки ленты в
  // начало.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(m_dev_conf.rewind_delay));
}

int TapeDev::getMemBufCurrValue() const noexcept {
  return m_mem_buf[m_mem_buf_index];
}

std::pair<std::vector<int>, size_t> TapeDev::getMemBufCopy() const noexcept {
  std::vector<int> copy(m_mem_buf, m_mem_buf + m_dev_conf.mem_buf_size);
  return std::make_pair(copy, m_mem_buf_index);
}

TapeDev::~TapeDev() noexcept {
  m_tape_file.close();
  delete[] m_mem_buf;
}
