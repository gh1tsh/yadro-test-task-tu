#include <filesystem>
#include <iostream>

#include "TapeDev.hpp"

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cout << "ОШИБКА: недопустимые аргументы командной строки. Программа "
                 "принимает 2 аргумента командной строки, получено: "
              << argc - 1 << "." << std::endl;
    return 1;
  }

  std::filesystem::path dev_path(argv[1]);

  if (!std::filesystem::exists(dev_path)) {
    std::cout << "ОШИБКА: файл '" << dev_path.string() << "' не существует."
              << std::endl;
    return 1;
  }

  std::filesystem::path out_tape_path(argv[2]);

  // Проверим, что существует директория, в которую будет записан итоговый
  // файл...
  std::filesystem::path t = out_tape_path.parent_path();
  if (!std::filesystem::exists(t)) {
    std::cout << "ОШИБКА: директория '" << t.string() << "' не существует."
              << std::endl;
    return 1;
  }

  std::cout << "Полученные аргументы командной строки:" << std::endl;
  std::cout << "Путь к устройству: " << dev_path.string() << std::endl;
  std::cout << "Путь к файлу выходной ленты: " << out_tape_path.string()
            << std::endl;

  std::filesystem::path configFilePath(
      "/home/ghitsh/workshop/work/test-tasks/yadro-task-2024-nov/"
      "yadro-test-task-tu/build/ProgramData/config/example_config.txt");

  const TapeDevConf tapeDevCfg = parseTapeConfigFile(configFilePath);

  std::cout << tapeDevCfg.to_string() << std::endl;

  TapeDev tapeDev(dev_path, tapeDevCfg, TapeDevOperationMode::ReadWrite);

  std::cout << tapeDev.read() << std::endl;
  std::cout << tapeDev.read() << std::endl;
  std::cout << tapeDev.read() << std::endl;
  tapeDev.shiftRight();
  std::cout << tapeDev.read() << std::endl;
  // tapeDev.write(42);
  // tapeDev.shiftLeft();
  // tapeDev.shiftLeft();
  std::cout << tapeDev.read() << std::endl;
}
