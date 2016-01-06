/*
 * ***** 框架使用说明 *****
 * 必需：
 * 1. 整个框架的介入点只有两个，init_master和exit_master函数，
 *    分别表示创建工作线程之前和工作线程结束之后
 * 2. ew_singleton_mng，这个是单例管理器对象，唯一的全局对象，框架使用了
 *    了一些单例对象，名称类似SGT_*，可从单例管理器中获取，SGT_THREAD_LIST是
 *    线程列表，加入到该列表的线程在init_master之后会执行
 *
 * 可选：
 * 1. 你完全可以删掉这个文件，然后自己从头实现init_master和exit_master，或者，
 *    在该文件基础上进行修改
 * 2. 封装了智能指针，::easywork::SmartPointer_::SmartPointer<T>，在类声明中
 *    使用了SMART_POINTABLE宏才能使用
 * 3. EwUtils类提供了一些实用的函数
 */
#include <arpa/inet.h>
#include <pthread.h>
#include <ev.h>

#include "smart_pointer.hpp"
#include "raii.hpp"
#include "easywork.hpp"


#define NUM_OF_WORKERS              2
#define SGT_THREAD_CONTEXTS         "easywork.thread_contexts"

// 线程组类型
#define THREAD_TYPE_SIGNAL          1
#define THREAD_TYPE_WORKER          2
#define THREAD_TYPE_BACKEND         3


static int lsn;
static pthread_spinlock_t accept_lock;


namespace example {
using ::std::map;
using ::easywork::Singletonable;


namespace ConnectionInfo_ {
class ConnectionInfo
{
SMART_POINTABLE;

public:
    int fd;
    struct sockaddr_in cli_addr;
};
}
typedef ConnectionInfo_::ConnectionInfo ConnectionInfo;
typedef ::easywork::SmartPointer_::SmartPointer<ConnectionInfo>
        ConnectionInfoPtr;
typedef map<int, ConnectionInfoPtr> ConnectionMap;


// 线程上下文
class ThreadContext;
typedef ::easywork::SmartPointer_::SmartPointer<ThreadContext>
        ThreadContextPtr;
class ThreadContextSignal;
typedef ::easywork::SmartPointer_::SmartPointer<ThreadContextSignal>
        ThreadContextSignalPtr;
class ThreadContextBackend;
typedef ::easywork::SmartPointer_::SmartPointer<ThreadContextBackend>
        ThreadContextBackendPtr;

class ThreadContext
{
SMART_POINTABLE;

public:
    ThreadContext(int thread_type, pthread_t tid)
        : type(thread_type), id(tid)
    {
        fprintf(stderr, "construct thread context\n");
    }

    virtual int Init(void) = 0;

    virtual ~ThreadContext(void)
    {
        fprintf(stderr, "destruct thread context\n");
    }

    int type; // 线程类型
    pthread_t id; // 线程id
};

class ThreadContextSignal : public ThreadContext
{
public:
    ThreadContextSignal(pthread_t tid)
        : ThreadContext(THREAD_TYPE_SIGNAL, tid)
    {}

    virtual int Init(void)
    {
        return 0;
    }

    virtual ~ThreadContextSignal(void)
    {}
};

class ThreadContextWorker : public ThreadContext
{
public:
    ThreadContextWorker(pthread_t tid)
        : ThreadContext(THREAD_TYPE_WORKER, tid),
          loop(NULL)
    {}

    virtual int Init(void)
    {
        this->loop = ::ev_loop_new(EVBACKEND_EPOLL);

        // todo: socketpair可以RAII封装一下
        if (-1 == ::socketpair(AF_UNIX, SOCK_STREAM, 0, this->channel)) {
            fprintf(stderr, "domain socket failed:%d\n", errno);

            return -1;
        }

        return 0;
    }

    virtual ~ThreadContextWorker(void)
    {
        static_cast<void>(::close(this->channel[0]));
        static_cast<void>(::close(this->channel[1]));

        ::ev_loop_destroy(this->loop);

        return;
    }

    struct ::ev_loop *loop; // 事件循环
    int channel[2]; // 域套接字，用于与信号处理线程间通信
    ConnectionMap connections; // 连接
    ThreadContextBackendPtr tback; // 后端线程上下文
};
typedef ::easywork::SmartPointer_::SmartPointer<ThreadContextWorker>
        ThreadContextWorkerPtr;

class ThreadContextBackend : public ThreadContext
{
public:
    ThreadContextBackend(pthread_t tid)
        : ThreadContext(THREAD_TYPE_BACKEND, tid),
          loop(NULL)
    {}

    virtual int Init(void)
    {
        this->loop = ::ev_loop_new(EVBACKEND_EPOLL);

        // todo: socketpair可以RAII封装一下
        if (-1 == ::socketpair(AF_UNIX, SOCK_STREAM, 0, this->channel)) {
            fprintf(stderr, "domain socket failed:%d\n", errno);

            return -1;
        }

        return 0;
    }

    virtual ~ThreadContextBackend(void)
    {
        static_cast<void>(::close(this->channel[0]));
        static_cast<void>(::close(this->channel[1]));

        ::ev_loop_destroy(this->loop);

        return;
    }

    struct ::ev_loop *loop; // 事件循环
    int channel[2]; // 域套接字，用于与工作线程间通信
};


class ThreadContextMap : public Singletonable
{
public:
    typedef map<pthread_t, ThreadContextPtr> Tid2Context;

    Tid2Context contexts;
};


void signal_thread_proc(void)
{
    int rslt;
    int signo;
    ::sigset_t ss;
    ThreadContextMap *tcm = reinterpret_cast<ThreadContextMap *>(
        ew_singleton_mng.GetObjInst(SGT_THREAD_CONTEXTS)
    );

    static_cast<void>(::sigemptyset(&ss));
    static_cast<void>(::sigfillset(&ss));

    while (true) {
        rslt = ::sigwait(&ss, &signo);
        if (rslt) {
            fprintf(stderr, "sigwait failed:%d\n", rslt);
            continue;
        }

        for (ThreadContextMap::Tid2Context::iterator it = tcm->contexts.begin();
             tcm->contexts.end() != it; ++it) {
            if (THREAD_TYPE_WORKER != it->second->type) {
                continue;
            }

            ThreadContextWorkerPtr tcw(static_cast<ThreadContextWorker *>(
                static_cast<ThreadContext *>(it->second)
            ));

            if (-1 == ::write(tcw->channel[0], &signo, sizeof(signo))) {
                fprintf(stderr, "write failed:%d\n", errno);
            }
        }

        // 退出信号处理线程
        if (SIGINT == signo) {
            break;
        }
    }

    return;
}


void accept_callback(struct ::ev_loop *loop, ::ev_io *w, int revents)
{
    int newfd;
    struct sockaddr_in cli_addr;
    socklen_t addrlen = sizeof(cli_addr);
    ThreadContextMap *tcm = reinterpret_cast<ThreadContextMap *>(
        ew_singleton_mng.GetObjInst(SGT_THREAD_CONTEXTS)
    );

    while (true) {
        newfd = ::accept(w->fd, (sockaddr *)&cli_addr, &addrlen);
        if (-1 == newfd) {
            if (ECONNABORTED == errno) {
                // 连接夭折
                continue;
            }

            if ((EAGAIN != errno) || (EWOULDBLOCK != errno)) {
                fprintf(stderr, "accept failed:%d\n", errno);
            }
            break;
        }
        fprintf(stderr, "%d:connection established\n", ::pthread_self());

        EW_SET_NONBLOCK(newfd);
        ::close(newfd);
    }

    return;
}


void worker_channel_callback(struct ::ev_loop *loop, ::ev_io *w, int revents)
{
    int signo;
    ::pthread_t tid = ::pthread_self();
    ThreadContextMap *tcm = reinterpret_cast<ThreadContextMap *>(
        ew_singleton_mng.GetObjInst(SGT_THREAD_CONTEXTS)
    );
    ThreadContextWorkerPtr tcw(static_cast<ThreadContextWorker *>(
        static_cast<ThreadContext *>(tcm->contexts[tid])
    ));

    if (-1 == ::read(tcw->channel[1], &signo, sizeof(signo))) {
        fprintf(stderr, "read failed:%d\n", errno);
        return;
    }

    if (SIGINT == signo) {
        ::ev_break(tcw->loop, EVBREAK_ONE);
    }

    return;
}


void backend_thread_proc(void)
{
    ::ev_io channel_watcher;
    ::pthread_t tid = ::pthread_self();
    ThreadContextBackend *this_ctx = new ThreadContextBackend(tid);
    ThreadContextMap *tcm = reinterpret_cast<ThreadContextMap *>(
        ew_singleton_mng.GetObjInst(SGT_THREAD_CONTEXTS)
    );

    ThreadContextPtr for_free(this_ctx);
    if (-1 == this_ctx->Init()) {
        return;
    }
    tcm->contexts[tid] = ThreadContextPtr(this_ctx);

    //ev_init(&channel_watcher, channel_callback);
    //ev_io_set(&channel_watcher, this_ctx->channel[1], EV_READ);
    //::ev_io_start(this_ctx->loop, &channel_watcher);

    static_cast<void>(::ev_run(this_ctx->loop, 0));

    fprintf(stderr, "thread exit:%lu\n", tid);

    return;
}


void worker_thread_proc(int lsn)
{
    ::ev_io accept_watcher;
    ::ev_io channel_watcher;
    ::pthread_t tid = ::pthread_self();
    ThreadContextMap *tcm = reinterpret_cast<ThreadContextMap *>(
        ew_singleton_mng.GetObjInst(SGT_THREAD_CONTEXTS)
    );

    // 后端线程
    ThreadContextBackend *backend_ctx = new ThreadContextBackend(tid);
    ::boost::thread tback(::boost::bind<void>(::example::backend_thread_proc));

    ThreadContextWorker *this_ctx = new ThreadContextWorker(tid);
    if (-1 == this_ctx->Init()) {
        return;
    }

    tcm->contexts[tid] = ThreadContextPtr(this_ctx);

    ev_init(&accept_watcher, accept_callback);
    ev_io_set(&accept_watcher, lsn, EV_READ);
    ev_init(&channel_watcher, worker_channel_callback);
    ev_io_set(&channel_watcher, this_ctx->channel[1], EV_READ);

    ::ev_io_start(this_ctx->loop, &accept_watcher);
    ::ev_io_start(this_ctx->loop, &channel_watcher);
    static_cast<void>(::ev_run(this_ctx->loop, 0));

    tback.join();

    return;
}


// test
class Test
{
SMART_POINTABLE;

public:
    Test() {fprintf(stderr, "Test:c\n");}
    ~Test() {fprintf(stderr, "Test:d\n");}
    int x;
};

}


namespace easywork {
using ::example::Test;
using ::example::ThreadContextMap;

int init_master(void)
{
    // test {{
    ::easywork::SmartPointer_::SmartPointer<Test> sp(new Test);
    ::easywork::SmartPointer_::SmartPointer<Test> sp2(sp);
    ::easywork::SmartPointer_::SmartPointer<Test> sp3(new Test);
    sp3 = sp;
    {
        ::easywork::SmartPointer_::SmartPointer<Test> sp4(sp);
    }
    // }} test

    ThreadContextMap *tcm = new ThreadContextMap();
    ew_singleton_mng.HoldObjInst(SGT_THREAD_CONTEXTS, tcm);

    // 添加监听
    lsn = EwUtils::InitTcpListen4();
    if (-1 == lsn) {
        fprintf(stderr, "listen failed\n");
        return -1;
    }

    // 初始化自旋锁
    if (::pthread_spin_init(&accept_lock, PTHREAD_PROCESS_PRIVATE)) {
        return -1;
    }

    ThreadList *tl = reinterpret_cast<ThreadList *>(
        ew_singleton_mng.GetObjInst(SGT_THREAD_LIST)
    );


    // 添加信号同步线程组
    ThreadSchema *ts_signal = new ThreadSchema();
    ts_signal->nthreads = 1;
    ts_signal->thread_proc = ::boost::bind<void>(
        ::example::signal_thread_proc
    );
    tl->AddThreadSchema(ts_signal);

    // 添加工作线程组
    ThreadSchema *ts_worker = new ThreadSchema();
    ts_worker->nthreads = NUM_OF_WORKERS;
    ts_worker->thread_proc = ::boost::bind<void>(
        ::example::worker_thread_proc, lsn
    );
    tl->AddThreadSchema(ts_worker);

    return 0;
}


void exit_master(void)
{
    if (::pthread_spin_destroy(&accept_lock)) {
        // log
    }
    EwUtils::ExitTcpListen(lsn);
    fprintf(stderr, "pid:%d\n", getpid());

    return;
}

}
