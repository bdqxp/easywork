#ifndef __EASYWORK__SINGLETON_MANAGER_HPP__
#define __EASYWORK__SINGLETON_MANAGER_HPP__


#include <list>
#include <map>
#include <string>

#include "common.hpp"


namespace easywork {

namespace Singletonable_ {
class Singletonable;
}
typedef Singletonable_::Singletonable Singletonable;

namespace SingletonManager_ {
class SingletonManager;
}
typedef SingletonManager_::SingletonManager SingletonManager;

} // end of namespace easywork


namespace easywork {

namespace Singletonable_ {
class Singletonable : public DenyCopyable
{
    friend class SingletonManager_::SingletonManager;

protected:
    explicit Singletonable(void) {}
    virtual ~Singletonable(void) {}
    virtual int Init(void)
    {
        return DenyCopyable::Init();
    }
};
}

namespace SingletonManager_ {
using ::std::map;
using ::std::list;
using ::std::string;

class SingletonManager : public DenyCopyable
{
public:
    explicit SingletonManager(void) {}
    virtual ~SingletonManager(void);
    virtual int Init(void)
    {
        return DenyCopyable::Init();
    }

public:
    void HoldObjInst(string const &name, Singletonable *inst);

    Singletonable *GetObjInst(string const &name) const
    {
        map<string, Singletonable *>::const_iterator it;

        it = this->objs_map.find(name);
        if (this->objs_map.end() == it) {
            return NULL;
        }

        return it->second;
    }

private:
    list<Singletonable *> objs_list;
    map<string, Singletonable *> objs_map;
};
} // end of namespace SingletonManager_

} // end of namespace easywork
#endif // __EASYWORK__SINGLETON_MANAGER_HPP__
