# yadro-test-task-tu

## Задание

Устройство хранения данных типа лента (Tape) предназначено для последовательной
записи и чтения данных. Считывающая/записывающая магнитная головка неподвижна 
во время чтения и записи, а лента имеет возможность двигаться в обоих 
направлениях. Запись и чтение информации возможны в ячейку ленты, на которой в 
данный момент находится магнитная головка. Перемещения ленты – затратная по 
времени операция – лента не предназначена для произвольного доступа. Имеется 
входная лента длины $N$ (где $N$ – велико), содержащая элементы типа `integer` 
($2^{32}$). Имеется выходная лента такой же длины. Необходимо записать в 
выходную ленту  отсортированные по возрастанию элементы с входной ленты. Есть 
ограничение по использованию оперативной памяти – не более $M$ байт ($M$ может 
быть меньше $N$, т.е. загрузить все данные с ленты в оперативную память не 
получится). Для реализации алгоритма можно использовать разумное количество 
временных лент, т.е. лент, на которых можно хранить какую-то временную 
информацию, необходимую в процессе работы алгоритма.

Создать проект С++, компилируемый в консольное приложение, которое реализует 
алгоритм сортировки данных с входной ленты на выходную. Необходимо сделать 
следующее:
- Определить интерфейс для работы с устройством типа лента.
- Написать класс, реализующий этот интерфейс и эмулирующий работу с лентой 
  посредством обычного файла. Должно быть возможно сконфигурировать (без 
  перекомпиляции – например, через внешний конфигурационный файл, который будет 
  прочитан на старте приложения) задержки по записи/чтению элемента с ленты, 
  перемотки ленты, и сдвига ленты на одну позицию.
- Файлы временных лент можно сохранять в директорию `tmp/`.
- Написать класс, реализующий алгоритм сортировки данных с входной ленты на 
  выходную.
- Консольное приложение должно принимать на вход имя входного и выходного файлов
   и производить сортировку.
- Желательно написать юнит-тесты.

## Сборка и тестирование

Для сборки и тестирования программы нужно выполнить следующие шаги.

1. Склонируйте репозиторий.

   ```bash
   git clone https://github.com/gh1tsh/yadro-test-task-tu.git
   ```

2. Перейдите в только что склонированный репозиторий.
   
   ```bash
   cd yadro-test-task-tu/
   ```

3. В корне проекта создайте директорию `build/` и перейдите в неё.
   
   ```bash
   mkdir ./build/ && cd ./build/
   ```

4. Выполните конфигурацию сборки проекта с помощью CMake.
   
   ```bash
   # Сборка с отладочной информацией (Debug-режим)
   cmake -DCMAKE_BUILD_TYPE=Debug ../TapeDataInterface/
   ```

   ```bash
   # Сборка с оптимизациями компилятора (Release-режим)
   cmake -DCMAKE_BUILD_TYPE=Release ../TapeDataInterface/
   ```

5. Выполните сборку проекта с помощью CMake.
   
   ```bash
   cmake --build .
   ```

6. Из директории `examples/`, которая расположена в корне проекта, скопируйте 
   директорию `ProgramData`, которая содержит файлы и директории, необходимые 
   для работы программы.
   
   ```bash
   cp -r ../examples/ProgramData/ .
   ```

7. Запуск тестов.
   
	```bash
	cd ./tests/
	```

	```bash
	./tapedatainterface_unit_tests
	```

8. Запуск программы.
   
   ```bash
   # из под директории ./build/
   ./tapedatainterface ./path/to/input/tape/file.txt ./path/to/output/tape/file.txt
   ```

## Технические подробности

### Файлы с некоторыми важными деталями, которые касаются работы программы

- [Формат файла ленты](./doc/tape_file_format.md)
- [Директории, используемые программой во время работы](./doc/program_dirs.md)

### Алгоритм сортировки

При разработке алгоритма первое, что было принято во внимание, - ограниченный
объём оперативной памяти устройства. При таких условиях в общем случае 
отсутствует возможность прочитать все значения находящиеся на ленте сразу, 
выполнить их сортировку и записать результат в файл выходной ленты. Поэтому был
применён подход, заключающийся в разделении всей последовательности значений на
части, их сортировке и сборке из полученных отсортированных частей целостной 
выходной ленты.

Пусть размер входной ленты $N$, а размер буфера памяти равен $M$, тогда
для максимального использования буфера памяти исходную ленту нужно будет
разбить на $\frac{N}{M}$ частей. Следовательно для хранения промежуточных 
результатов потребуется $\frac{N}{M}$ временных лент.

Алгоритм предусматривает также случай, при котором есть возможность сразу 
записать в память все значения из входной ленты. В такой ситуации сразу 
выполняется сортировка и запись результатов на выходную ленту.

Разбиение исходной ленты на части выполняется на подготовительном этапе 
алгоритма (метод `TapeSorter::setup()`). Сам алгоритм сортировки состоит из
прямого хода (метод `TapeSorter::forward_pass()`) и обратного хода (метод
`TapeSorter::beckward_pass()`).

Во время прямого хода алгоритма данные с каждой временной ленты считываются, 
сортируются и записываются в соответствующие файлы. Также подсчитывается общее 
количество значений на входной ленте а также составляется массив, содержащий 
количество значений на каждой временной ленте.

Обратный ход заключается в циклическом обходе всех временных лент, на каждой
итерации которого из текущих значений временных лент выбирается наименьшее и
записывается на выходную ленту (для отслеживания текущих значений каждой 
временной ленты заводится отдельный массив). Цикл выполняется, пока все значения
со всех временных лент не будут обработаны и помещены на выходную ленту.