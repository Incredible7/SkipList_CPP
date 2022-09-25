#include "skiplist.h"
#define FILE_PATH "./store/DumpFile"

auto main() -> int {
    SkipList<int, std::string> skipList(6);
    skipList.insert_element(35, "堆");
    skipList.insert_element(1, "晚");
    skipList.insert_element(2, "景");
    skipList.insert_element(5, "琼");
    skipList.insert_element(4, "落");
    skipList.insert_element(20, "翠");
    skipList.insert_element(7, "杯");
    skipList.insert_element(10, "，");
    skipList.insert_element(11, "照");
    skipList.insert_element(15, "云");
    skipList.insert_element(12, "眼");
    skipList.insert_element(17, "山");
    skipList.insert_element(22, "作");

    std::cout << "skipList size:" << skipList.size() << std::endl;

    skipList.dump_file();

    // skipList.load_file();

    skipList.search_element(5);
    skipList.search_element(17);

    skipList.display_list();

    skipList.delete_element(3);
    skipList.delete_element(7);

    std::cout << "skipList size:" << skipList.size() << std::endl;

    skipList.display_list();
}