#ifndef __EASYWORK__EASYWORK_HPP__
#define __EASYWORK__EASYWORK_HPP__


#include <fcntl.h>

#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include "singleton_manager.hpp"


#define SRC2STR_ARG(arg)    #arg
#define SRC2STR(src)        SRC2STR_ARG(src)


// 框架单例名称
#define SGT_EWCONF          "easywork.ew_conf"
#define SGT_THREAD_LIST     "easywork.thread_list"


namespace easywork {

namespace EwConf_ {
class EwConf;
}
typedef EwConf_::EwConf EwConf;

namespace EwUtils_ {
class EwUtils;
}
typedef EwUtils_::EwUtils EwUtils;

namespace ThreadSchema_ {
struct ThreadSchema;
}
typedef ThreadSchema_::ThreadSchema ThreadSchema;

namespace ThreadList_ {
class ThreadList;
}
typedef ThreadList_::ThreadList ThreadList;

namespace Main_ {
class Main;
}
typedef Main_::Main Main;

}


namespace easywork {

// 框架配置类
namespace EwConf_ {
class EwConf : public Singletonable
{
public:
    explicit EwConf(void)
    {
    }

    virtual ~EwConf(void)
    {}

    virtual int Init(void)
    {
        int ret = Singletonable::Init();

        if (ret) {
            return ret;
        }

        return 0;
    }

    int unused;
};
}

// 可选用的一些工具
namespace EwUtils_ {
class EwUtils
{
public:
    static int InitTcpListen4();
    static int InitTcpListen6();
    static void ExitTcpListen(int fd);

private:
    explicit EwUtils(void);
    ~EwUtils(void);
};
}


// 线程逻辑体
namespace ThreadSchema_ {
using ::boost::thread_group;
using ::boost::function;

struct ThreadSchema
{
    thread_group tg;
    int nthreads;
    function<void (void)> thread_proc;
};
}


// 线程列表
namespace ThreadList_ {
using ::std::list;
using ::boost::thread_group;

class ThreadList : public Singletonable
{
public:
    explicit ThreadList(void)
    {
        return;
    }

    virtual ~ThreadList(void)
    {
        while (this->thread_set.size()) {
            ThreadSchema *p = this->thread_set.front();
            this->thread_set.pop_front();
            delete p;
        }

        return;
    }
    virtual int Init(void)
    {
        int ret = Singletonable::Init();

        if (ret) {
            return ret;
        }

        return 0;
    }


public:
    void AddThreadSchema(ThreadSchema *ts)
    {
        this->thread_set.push_back(ts);
    }

    const list<ThreadSchema *> *GetThreadSet(void)
    {
        return &this->thread_set;
    }


private:
    list<ThreadSchema *> thread_set;
};
}

}

extern ::easywork::SingletonManager ew_singleton_mng;
#endif // __EASYWORK__EASYWORK_HPP__
