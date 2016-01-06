#include "singleton_manager.hpp"


namespace easywork {

namespace SingletonManager_ {
SingletonManager::~SingletonManager(void)
{
    if (this->objs_map.size() != this->objs_list.size()) {
        ::abort();
    }

    while (this->objs_list.size()) {
        Singletonable *p = this->objs_list.front();
        this->objs_list.pop_front();
        delete p;
    }
}

void SingletonManager::HoldObjInst(string const &name, Singletonable *inst)
{
    if (this->objs_map.end() != this->objs_map.find(name)) {
        ::abort();
    }

    this->objs_list.push_front(inst);
    this->objs_map[name] = inst;
}
}

}
