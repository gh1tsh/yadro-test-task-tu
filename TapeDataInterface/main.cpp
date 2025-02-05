#include <filesystem>
#include <iostream>

#include "TapeDev.hpp"
#include "TapeDevConfig.hpp"
#include "TapeSorter.hpp"

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cout << "ОШИБКА: недопустимые аргументы командной строки. Программа "
                 "принимает 2 аргумента командной строки, получено: "
              << argc - 1 << "." << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "\t\t--- Программа для сортировки данных на ленте ---\n\n\n";

  // Проверяем, что программа запущена из директории ./yadro-test-task-tu/build.
  std::filesystem::path program_working_dir_path = std::filesystem::current_path();
  if (program_working_dir_path.filename() != "build") {
    std::cout << "ОШИБКА: программа должна быть запущена из директории "
                 "./yadro-test-task-tu/build."
              << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Проверка наличия директории ProgramData...";

  // Проверяем наличие директории ProgramData в директории ./yadro-test-task-tu/build.
  std::filesystem::path program_data_dir_path = program_working_dir_path / "ProgramData";
  if (!std::filesystem::exists(program_data_dir_path)) {
    std::cout << "\n\nОШИБКА: директория ProgramData не найдена в '" +
                     program_working_dir_path.string() + "'."
              << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << " Успешно" << std::endl;

  std::cout << std::endl << "Проверка наличия необходимых директорий в ProgramData...";

  // Проверяем наличие необходимых директорий, config/, var/, var/tmp/, в директории ProgramData.
  std::filesystem::path program_config_dir_path = program_data_dir_path / "config";
  if (!std::filesystem::exists(program_config_dir_path)) {
    std::cout << "\n\nОШИБКА: директория config не найдена в '" + program_data_dir_path.string() +
                     "'."
              << std::endl;
    return EXIT_FAILURE;
  }

  std::filesystem::path program_var_dir_path = program_data_dir_path / "var";
  if (!std::filesystem::exists(program_var_dir_path)) {
    std::cout << "\n\nОШИБКА: директория var не найдена в '" + program_data_dir_path.string() + "'."
              << std::endl;
    return EXIT_FAILURE;
  }

  std::filesystem::path program_var_tmp_dir_path = program_var_dir_path / "tmp";
  if (!std::filesystem::exists(program_var_tmp_dir_path)) {
    std::cout << "\n\nОШИБКА: директория tmp не найдена в '" + program_var_dir_path.string() + "'."
              << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << " Успешно" << std::endl;

  std::filesystem::path in_tape_file_path(argv[1]);

  if (!std::filesystem::exists(in_tape_file_path)) {
    std::cout << "\nОШИБКА: файл '" << in_tape_file_path.string() << "' не существует."
              << std::endl;
    return EXIT_FAILURE;
  }

  std::filesystem::path out_tape_file_path(argv[2]);

  // Проверим, что существует директория, в которую будет записан выходной файл.
  std::filesystem::path t = out_tape_file_path.parent_path();
  if (!std::filesystem::exists(t)) {
    std::cout << "\nОШИБКА: директория '" << t.string() << "' не существует." << std::endl;
    return EXIT_FAILURE;
  }

  std::filesystem::path config_file_path("./ProgramData/config/device_config.txt");

  std::cout << "\nПроверка наличия файла конфигурации устройства...";

  // Проверим наличие файла конфигурации устройства.
  if (!std::filesystem::exists(config_file_path)) {
    std::cout << "\nОШИБКА: файл конфигурации устройства '" << config_file_path.string()
              << "' не найден." << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << " Успешно" << std::endl;

  std::cout << "Полученные аргументы командной строки:" << std::endl;
  std::cout << "\tпуть к файлу входной ленты: " << in_tape_file_path.string() << std::endl;
  std::cout << "\tпуть к файлу выходной ленты: " << out_tape_file_path.string() << std::endl;

  std::cout << "\nПарсинг файла конфигурации устройства...";

  TapeDevConfig tape_dev_config;
  try {
    tape_dev_config = parseTapeConfigFile(config_file_path);
  } catch (const std::runtime_error& e) {
    std::cout << "\n\nОШИБКА: не удалось выполнить парсинг файла конфигурации устройства.\n"
              << "Причина: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << " Успешно" << std::endl;

  std::cout << std::endl << "Конфигурация устройства:" << std::endl;

  std::cout << tape_dev_config.to_string() << std::endl << std::endl;

  TapeDev tape_dev(in_tape_file_path, tape_dev_config, TapeDevOperationMode::ReadWrite);

  TapeSorter tapeSorter(tape_dev, in_tape_file_path, out_tape_file_path, program_data_dir_path);

  std::cout << "Выполняется сортировка ленты...";

  try {
    tapeSorter.sort();
  } catch (const std::runtime_error& e) {
    std::cout << "\n\nОШИБКА: " + std::string(e.what()) << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << " Успешно" << std::endl;

  std::cout << std::endl
            << "Результаты сортировки записаны в файл '" << out_tape_file_path.string() << "'."
            << std::endl;

  std::cout << "Завершение работы программы..." << std::endl;
}
