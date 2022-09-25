#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>

#define STORE_FILE "store/dumpFile"

std::mutex  mtx;              // 关键部分的互斥锁
std::string delimiter = ":";  // 定界符

// Node类定义
template <typename K, typename V>
class Node {
public:
    Node() {}
    Node(K k, V v, int);
    ~Node();

    K    get_key() const;
    V    get_value() const;
    void set_value(V);

    // 线性数组，用于保存指向不同级别的下一个节点的指针
    Node<K, V>** forward;
    int          node_level;

private:
    K key;
    V value;
};

template <typename K, typename V>
Node<K, V>::Node(const K k, const V v, int level) {
    this->key = k;
    this->value = v;
    this->node_level = level;

    // 开辟容量为“级别+1”的指针数组，因为级别为“0-level”
    this->forward = new Node<K, V>*[level + 1];
    // 数组初始化
    memset(this->forward, 0, sizeof(Node<K, V>*) * (level + 1));
}

template <typename K, typename V>
Node<K, V>::~Node<K, V>() {
    delete[] forward;
}

template <typename K, typename V>
K Node<K, V>::get_key() const {
    return key;
};

template <typename K, typename V>
V Node<K, V>::get_value() const {
    return value;
};
template <typename K, typename V>
void Node<K, V>::set_value(V value) {
    this->value = value;
};

// 跳表的类模板
template <typename K, typename V>
class SkipList {
public:
    SkipList(int);
    ~SkipList();

    int         get_random_level();  // 随机获得层级
    Node<K, V>* create_node(K, V, int);
    int         insert_element(K, V);
    void        display_list();
    bool        search_element(K);
    void        delete_element(K);
    void        dump_file();
    void        load_file();
    int         size();

private:
    void get_key_value_from_string(const std::string& str, std::string* key,
                                   std::string* value);
    bool is_valid_string(const std::string& str);

private:
    int         _max_level;        // 跳表能构建的层数上限
    int         _skip_list_level;  // 当前跳表层数
    Node<K, V>* _header;           // 指向头结点的指针

    std::ofstream _file_writer;
    std::ifstream _file_reader;

    int _element_count;  // 跳表当前元素数量
};

// 创建新节点
template <typename K, typename V>
Node<K, V>* SkipList<K, V>::create_node(const K k, const V v, int level) {
    Node<K, V>* n = new Node<K, V>(k, v, level);
    return n;
}

// 将给出的key、value作为键插入
// 元素已存在返回1，元素成功插入返回0
/*
                           +------------+
                           |  insert 50 |
                           +------------+
level 4     +-->1+                                                      100
                 |
                 |                      insert +----+
level 3         1+-------->10+---------------> | 50 |          70       100
                                               |    |
                                               |    |
level 2         1          10         30       | 50 |          70       100
                                               |    |
                                               |    |
level 1         1    4     10         30       | 50 |          70       100
                                               |    |
                                               |    |
level 0         1    4   9 10         30   40  | 50 |  60      70       100
                                               +----+

*/
template <typename K, typename V>
int SkipList<K, V>::insert_element(const K key, const V value) {
    mtx.lock();
    Node<K, V>* current = this->_header;

    // 创建 update 数组并且初始化
    // update 数组将存储那些 node->forward[i] 需要被操作的 node 节点
    // 用人话讲就是 update 记录新节点在每一层的上一个节点
    // 也就是新节点要插到哪个节点后面
    Node<K, V>* update[_max_level + 1];
    memset(update, 0, sizeof(Node<K, V>*) * (_max_level + 1));

    // 从最高层开始遍历
    for (int i = _skip_list_level; i >= 0; --i) {
        while (current->forward[i] != nullptr &&
               current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }
    /*
        这里我解释一下：就拿上面的示例来说，update[4] 就是 1（key 为 1
        的节点）， update[3] 就是 10，update[2] 就是 30， update[1]
        就是 30，update[0] 就是 40。
    */

    // 记录完毕后当前节点回到最低层级的下一个节点
    // 因为如果已经插入该键值，那么它的层级不确定，一定要到最低层级
    current = current->forward[0];
    // 如果当前节点键值与插入键值相等，返回
    if (current != nullptr && current->get_key() == key) {
        std::cout << "key: " << key << " exists!" << std::endl;
        mtx.unlock();
        return 1;
    }

    // 如果 current==nullptr 说明我们记录时已经到了最后一个节点
    // 如果 current->key!=key 说明我们需要在update[0] 和 current 之间插入
    if (current == nullptr || current->get_key() != key) {
        // 为当前节点生成一个随机层级
        int random_level = get_random_level();

        // 如果 random_level 大于跳表的当前层级
        // 就让 update[level] 指向头结点
        if (random_level > _skip_list_level) {
            for (int i = _skip_list_level + 1; i < random_level + 1; ++i) {
                update[i] = _header;
            }
            _skip_list_level = random_level;  // 更新跳表层数
        }

        // 生成一个新节点并赋值
        Node<K, V>* inserted_node = create_node(key, value, random_level);
        // 插入
        for (int i = 0; i < random_level; ++i) {
            inserted_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = inserted_node;
        }
        std::cout << "Successfully inserted key: " << key
                  << ", value: " << value << std::endl;
        ++_element_count;
    }

    mtx.unlock();
    return 0;
}

// 展示跳表
template <typename K, typename V>
void SkipList<K, V>::display_list() {
    std::cout << "\n*****SKIP LIST*****\n";
    for (int i = 0; i <= _skip_list_level; ++i) {
        Node<K, V>* node = this->_header->forward[i];
        std::cout << "Level " << i << ": ";
        while (node != nullptr) {
            std::cout << node->get_key() << ": " << node->get_value()
                      << ";";
            node = node->forward[i];
        }
        std::cout << std::endl;
    }
}

// 转存数据到文件
template <typename K, typename V>
void SkipList<K, V>::dump_file() {
    std::cout << "DUMP FILE-----------------" << std::endl;
    _file_writer.open(STORE_FILE);
    Node<K, V>* node = this->_header->forward[0];

    while (node != nullptr) {
        _file_writer << node->get_key() << ": " << node->get_value()
                     << "\n";
        std::cout << node->get_key() << ": " << node->get_value() << ";\n";
        node = node->forward[0];
    }
}

// 从磁盘加载数据（读文件）
template <typename K, typename V>
void SkipList<K, V>::load_file() {
    _file_reader.open(STORE_FILE);
    std::cout << "LOAD FILE-----------------" << std::endl;
    std::string line;

    // 这里之所以用到指针，是因为方便从文件里赋值
    std::string* key = new std::string();
    std::string* value = new std::string();
    while (getline(_file_reader, line)) {
        get_key_value_from_string(line, key, value);
        if (key->empty() || value->empty()) {
            continue;
        }
        insert_element(*key, *value);
        std::cout << "key: " << *key << ", value: " << *value << std::endl;
    }
    _file_reader.close();
}

// 获取当前跳表容纳元素数量
template <typename K, typename V>
int SkipList<K, V>::size() {
    return _element_count;
}

// 解析语句，将 key 与 value 提取出来
template <typename K, typename V>
void SkipList<K, V>::get_key_value_from_string(const std::string& str,
                                               std::string*       key,
                                               std::string*       value) {
    if (!is_valid_string(str)) {
        return;
    }
    *key = str.substr(0, str.find(delimiter));
    *value = str.substr(str.find(delimiter) + 1, str.length());
}

// 判断字符串（键值）是否有效
template <typename K, typename V>
bool SkipList<K, V>::is_valid_string(const std::string& str) {
    if (str.empty())
        return false;
    if (str.find(delimiter) == std::string::npos)
        return false;
    return true;
}

// 删除
template <typename K, typename V>
void SkipList<K, V>::delete_element(K key) {
    mtx.lock();
    Node<K, V>* current = this->_header;
    Node<K, V>* update[_max_level + 1];
    memset(update, 0, sizeof(Node<K, V>*) * (_max_level + 1));

    // 从最高层开始
    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] != NULL &&
               current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    // 和之前一样，回到最底层
    current = current->forward[0];
    if (current != nullptr && current->get_key() == key) {
        // 从最底层开始依次删除每层的 current 节点
        for (int i = 0; i <= _skip_list_level; ++i) {
            // 如果在第 i 层，next node不是目标节点，直接break
            if (update[i]->forward[i] != current)
                break;

            update[i]->forward[i] = current->forward[i];
        }

        // 移除没有元素的层级
        while (_skip_list_level > 0 &&
               _header->forward[_skip_list_level] == 0) {
            --_skip_list_level;
        }

        std::cout << "Successfully deleted key: " << key << std::endl;
        --_element_count;
    }
    mtx.unlock();
    return;
}

// 搜索
template <typename K, typename V>
bool SkipList<K, V>::search_element(K key) {
    std::cout << "SEARCH-----------------" << std::endl;
    Node<K, V>* current = _header;

    //操作原理同上
    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] &&
               current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
    }

    current = current->forward[0];

    if (current and current->get_key() == key) {
        std::cout << "Found key: " << key
                  << ", value: " << current->get_value() << std::endl;
        return true;
    }

    std::cout << "Not Found Key:" << key << std::endl;
    return false;
}

// 构建跳表
template <typename K, typename V>
SkipList<K, V>::SkipList(int max_level) {
    this->_max_level = max_level;
    this->_skip_list_level = 0;
    this->_element_count = 0;

    // create header node and initialize key and value to null
    K k;
    V v;
    this->_header = new Node<K, V>(k, v, _max_level);
};

template <typename K, typename V>
SkipList<K, V>::~SkipList() {
    if (_file_writer.is_open()) {
        _file_writer.close();
    }
    if (_file_reader.is_open()) {
        _file_reader.close();
    }
    delete _header;
}

template <typename K, typename V>
int SkipList<K, V>::get_random_level() {
    int k = 1;
    while (rand() % 2) {
        k++;
    }
    k = (k < _max_level) ? k : _max_level;
    return k;
};
    // vim: et tw=100 ts=4 sw=4 cc=120

#endif