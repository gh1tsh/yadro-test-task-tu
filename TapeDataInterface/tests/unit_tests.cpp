#include <gtest/gtest.h>

#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "../TapeDev.hpp"
#include "../TapeDevConfig.hpp"
#include "../TapeDevExceptions.hpp"
#include "../TapeSorter.hpp"

class TapeDataInterfaceTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    tapes_dir = std::filesystem::path("../../TapeDataInterface/tests/tests-data/tapes/");
    output_dir = std::filesystem::path("../../TapeDataInterface/tests/tests-data/out/");
    // Создаем конфигурацию устройства
    TapeDevConfig config("../../TapeDataInterface/tests/tests-data/device_config.txt", 5, 0, 0, 0,
                         0);
    // Инициализируем объект TapeDev
    tape_dev = new TapeDev(tapes_dir / "simple_tape.txt", config, TapeDevOperationMode::Read);
    tape_sorter = new TapeSorter(*tape_dev, "input_tape.txt", "output_tape.txt",
                                 "../../TapeDataInterface/tests/tests-data/");
  }

  static void TearDownTestCase() {
    afterTestCleanup();
    delete tape_sorter;
    delete tape_dev;
  }

  std::string getFileContentAsStr(const std::filesystem::path& file_path) {
    std::ifstream file(file_path);
    std::string content;
    std::getline(file, content);
    return content;
  }

  static void afterTestCleanup() {
    std::filesystem::remove(output_dir / "write_blank_test_tape.txt");
    std::filesystem::remove(output_dir / "sort_simple_sorted_test_tape.txt");
    std::filesystem::remove(output_dir / "sort_hard_sorted_test_tape.txt");
    std::filesystem::remove(output_dir / "sort_simple_test_tape.txt");
    std::filesystem::remove(output_dir / "sort_medium_test_tape.txt");
    std::filesystem::remove(output_dir / "sort_hard_test_tape.txt");
    std::filesystem::remove(output_dir / "sort_empty_tape_test.txt");
  }

  static TapeDev* tape_dev;
  static TapeSorter* tape_sorter;
  static std::filesystem::path tapes_dir;
  static std::filesystem::path output_dir;
};

// Инициализация статических членов класса
TapeDev* TapeDataInterfaceTest::tape_dev = nullptr;
TapeSorter* TapeDataInterfaceTest::tape_sorter = nullptr;
std::filesystem::path TapeDataInterfaceTest::tapes_dir;
std::filesystem::path TapeDataInterfaceTest::output_dir;

TEST_F(TapeDataInterfaceTest, TapeDevReadValueTest) {
  tape_dev->replaceTape(tapes_dir / "simple_tape.txt", TapeDevOperationMode::Read);
  EXPECT_EQ(tape_dev->read(), 2);
}

TEST_F(TapeDataInterfaceTest, TapeDevShiftRightThenReadValueTest) {
  tape_dev->replaceTape(tapes_dir / "simple_tape.txt", TapeDevOperationMode::Read);
  tape_dev->shiftRight();
  EXPECT_EQ(tape_dev->read(), 1);
}

TEST_F(TapeDataInterfaceTest, TapeDev3ShiftRightShiftLeftThenReadValueTest) {
  tape_dev->replaceTape(tapes_dir / "simple_tape.txt", TapeDevOperationMode::Read);
  tape_dev->shiftRight();
  tape_dev->shiftRight();
  tape_dev->shiftRight();
  tape_dev->shiftLeft();
  EXPECT_EQ(tape_dev->read(), 9);
}

TEST_F(TapeDataInterfaceTest, TapeDev4ShiftRightThenShiftLeftToTheStartOfTapeTest) {
  tape_dev->replaceTape(tapes_dir / "simple_tape.txt", TapeDevOperationMode::Read);
  tape_dev->shiftRight();
  tape_dev->shiftRight();
  tape_dev->shiftRight();
  tape_dev->shiftRight();
  tape_dev->shiftLeft();
  tape_dev->shiftLeft();
  tape_dev->shiftLeft();
  tape_dev->shiftLeft();
  EXPECT_EQ(tape_dev->atStartOfTape(), true);
}

TEST_F(TapeDataInterfaceTest, TapeDevShiftRightToTheEndOfTapeTest) {
  tape_dev->replaceTape(tapes_dir / "simple_tape.txt", TapeDevOperationMode::Read);
  for (size_t i = 0; i < 10; ++i) {
    tape_dev->shiftRight();
  }
  EXPECT_EQ(tape_dev->atEndOfTape(), true);
}

TEST_F(TapeDataInterfaceTest, TapeDevRewindTest) {
  tape_dev->replaceTape(tapes_dir / "simple_tape.txt", TapeDevOperationMode::Read);
  tape_dev->shiftRight();
  tape_dev->shiftRight();
  tape_dev->shiftRight();
  tape_dev->shiftRight();
  tape_dev->rewind();
  EXPECT_EQ(tape_dev->atStartOfTape(), true);
}

TEST_F(TapeDataInterfaceTest, TapeDevReplaceTapeGoodTapeTest) {
  tape_dev->replaceTape(tapes_dir / "short_sorted_tape.txt", TapeDevOperationMode::Read);
  EXPECT_EQ(tape_dev->read(), 1);
}

TEST_F(TapeDataInterfaceTest, TapeDevReplaceTapeBadTapeTest) {
  EXPECT_THROW(tape_dev->replaceTape(tapes_dir / "unknown_tape.txt", TapeDevOperationMode::Read),
               BadTapeException);
}

TEST_F(TapeDataInterfaceTest, TapeDevWriteModeWriteValueOnBlankTapeTest) {
  tape_dev->replaceTape(output_dir / "write_blank_test_tape.txt", TapeDevOperationMode::Write);
  tape_dev->write(42);
  std::string file_content = getFileContentAsStr(output_dir / "write_blank_test_tape.txt");
  EXPECT_EQ(file_content, "42");
}

TEST_F(TapeDataInterfaceTest, TapeDevAppendModeWriteValueTest) {
  tape_dev->replaceTape(output_dir / "write_blank_test_tape.txt", TapeDevOperationMode::Append);
  tape_dev->write(100);
  std::string file_content = getFileContentAsStr(output_dir / "write_blank_test_tape.txt");
  EXPECT_EQ(file_content, "42 100");
}

TEST_F(TapeDataInterfaceTest, TapeDevReadWriteModeRewriteValueTest) {
  tape_dev->replaceTape(output_dir / "write_blank_test_tape.txt", TapeDevOperationMode::ReadWrite);
  tape_dev->shiftRight();
  tape_dev->write(42);
  std::string file_content = getFileContentAsStr(output_dir / "write_blank_test_tape.txt");
  EXPECT_EQ(file_content, "42 42");
}

TEST_F(TapeDataInterfaceTest, TapeDevReadWriteModeShiftRightLeftThenWriteValueTest) {
  tape_dev->replaceTape(output_dir / "write_blank_test_tape.txt", TapeDevOperationMode::ReadWrite);
  tape_dev->shiftRight();
  tape_dev->shiftLeft();
  tape_dev->write(888);
  std::string file_content = getFileContentAsStr(output_dir / "write_blank_test_tape.txt");
  EXPECT_EQ(file_content, "888 42");
}

TEST_F(TapeDataInterfaceTest, TapeDevReadWriteModeWriteValueShiftRightWriteValueTest) {
  tape_dev->replaceTape(output_dir / "write_blank_test_tape.txt", TapeDevOperationMode::ReadWrite);
  tape_dev->write(55);
  tape_dev->shiftRight();
  tape_dev->write(55);
  std::string file_content = getFileContentAsStr(output_dir / "write_blank_test_tape.txt");
  EXPECT_EQ(file_content, "55 55");
}

TEST_F(TapeDataInterfaceTest, TapeDevReadWriteModeReadShiftRightReadTest) {
  tape_dev->replaceTape(tapes_dir / "simple_tape.txt", TapeDevOperationMode::ReadWrite);
  std::string res;
  int val = tape_dev->read();
  res += std::to_string(val);
  tape_dev->shiftRight();
  val = tape_dev->read();
  res += " " + std::to_string(val);
  EXPECT_EQ(res, "2 1");
}

TEST_F(TapeDataInterfaceTest, TapeDevReadModeReadValueOnBlankTapeTest) {
  tape_dev->replaceTape(tapes_dir / "empty_tape.txt", TapeDevOperationMode::Read);
  EXPECT_THROW(tape_dev->read(), BadTapeException);
}

TEST_F(TapeDataInterfaceTest, TapeDevReadModeInvalidWriteOperationTest) {
  tape_dev->replaceTape(tapes_dir / "short_sorted_tape.txt", TapeDevOperationMode::Read);
  EXPECT_THROW(tape_dev->write(42), InvalidOperationException);
}

TEST_F(TapeDataInterfaceTest, TapeDevWriteModeInvalidReadOperationTest) {
  tape_dev->replaceTape(output_dir / "write_blank_test_tape.txt", TapeDevOperationMode::Write);
  EXPECT_THROW(tape_dev->read(), InvalidOperationException);
}

TEST_F(TapeDataInterfaceTest, TapeSorterSortEmptyTapeTest) {
  tape_dev->replaceTape(tapes_dir / "empty_tape.txt", TapeDevOperationMode::Read);
  delete tape_sorter;
  tape_sorter = new TapeSorter(*tape_dev, tapes_dir / "empty_tape.txt",
                               output_dir / "sort_empty_tape_test.txt",
                               "../../TapeDataInterface/tests/tests-data/");
  EXPECT_THROW(tape_sorter->sort(), std::runtime_error);
}

TEST_F(TapeDataInterfaceTest, TapeSorterSortSimpleSortedTapeTest) {
  tape_dev->replaceTape(tapes_dir / "short_sorted_tape.txt", TapeDevOperationMode::Read);
  delete tape_sorter;
  tape_sorter = new TapeSorter(*tape_dev, tapes_dir / "short_sorted_tape.txt",
                               output_dir / "sort_simple_sorted_test_tape.txt",
                               "../../TapeDataInterface/tests/tests-data/");
  tape_sorter->sort();
  std::string file_content = getFileContentAsStr(output_dir / "sort_simple_sorted_test_tape.txt");
  EXPECT_EQ(file_content, "1 2 3 4 5 6 7 8 9 10");
}

TEST_F(TapeDataInterfaceTest, TapeSorterSortHardSortedTapeTest) {
  tape_dev->replaceTape(tapes_dir / "long_sorted_tape.txt", TapeDevOperationMode::Read);
  delete tape_sorter;
  tape_sorter = new TapeSorter(*tape_dev, tapes_dir / "long_sorted_tape.txt",
                               output_dir / "sort_hard_sorted_test_tape.txt",
                               "../../TapeDataInterface/tests/tests-data/");
  tape_sorter->sort();
  std::string file_content = getFileContentAsStr(output_dir / "sort_hard_sorted_test_tape.txt");
  EXPECT_EQ(file_content,
            "1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 "
            "32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 "
            "60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 "
            "88 89 90 91 92 93 94 95 96 97 98 99 100");
}

TEST_F(TapeDataInterfaceTest, TapeSorterSortSimpleTapeTest) {
  tape_dev->replaceTape(tapes_dir / "simple_tape.txt", TapeDevOperationMode::Read);
  delete tape_sorter;
  tape_sorter = new TapeSorter(*tape_dev, tapes_dir / "simple_tape.txt",
                               output_dir / "sort_simple_test_tape.txt",
                               "../../TapeDataInterface/tests/tests-data/");
  tape_sorter->sort();
  std::string file_content = getFileContentAsStr(output_dir / "sort_simple_test_tape.txt");
  EXPECT_EQ(file_content, "1 2 3 4 5 6 7 8 9 10");
}

TEST_F(TapeDataInterfaceTest, TapeSorterSortMediumTapeTest) {
  tape_dev->replaceTape(tapes_dir / "medium_tape.txt", TapeDevOperationMode::Read);
  delete tape_sorter;
  tape_sorter = new TapeSorter(*tape_dev, tapes_dir / "medium_tape.txt",
                               output_dir / "sort_medium_test_tape.txt",
                               "../../TapeDataInterface/tests/tests-data/");
  tape_sorter->sort();
  std::string file_content = getFileContentAsStr(output_dir / "sort_medium_test_tape.txt");
  EXPECT_EQ(file_content,
            "2 2 2 3 5 6 8 9 9 10 11 12 14 14 14 17 18 18 18 21 21 21 22 22 22 24 24 25 25 27 27 "
            "29 31 33 34 34 36 36 38 39 39 42 43 45 46 46 47 47 49 50");
}

TEST_F(TapeDataInterfaceTest, TapeSorterSortHardTapeTest) {
  tape_dev->replaceTape(tapes_dir / "hard_tape.txt", TapeDevOperationMode::Read);
  delete tape_sorter;
  tape_sorter =
      new TapeSorter(*tape_dev, tapes_dir / "hard_tape.txt", output_dir / "sort_hard_test_tape.txt",
                     "../../TapeDataInterface/tests/tests-data/");
  tape_sorter->sort();
  std::string file_content = getFileContentAsStr(output_dir / "sort_hard_test_tape.txt");
  std::string expected(
      "1 3 5 5 6 7 9 10 10 11 12 13 14 16 16 16 17 17 18 18 19 20 21 22 23 24 24 25 25 26 27 28 29 "
      "30 31 31 32 32 33 35 35 36 36 38 38 39 39 40 41 45 47 47 48 48 49 54 55 55 56 59 60 61 62 "
      "62 63 63 65 67 69 70 70 73 74 74 76 76 78 79 79 79 80 80 81 83 84 84 84 85 85 85 87 88 88 "
      "90 93 94 95 96 99 100");
  EXPECT_EQ(file_content, expected);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}