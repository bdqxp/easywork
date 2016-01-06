#include <signal.h>
#include <arpa/inet.h>

#include "raii.hpp"
#include "easywork.hpp"


::easywork::SingletonManager ew_singleton_mng;


namespace easywork {
extern int init_master(void);
extern void exit_master(void);


// ::easywork::EwUtils_
namespace EwUtils_ {
int EwUtils::InitTcpListen4()
{
    int fd;
    int nopt;
    struct sockaddr_in addr;

    if (-1 == (fd = ::socket(AF_INET, SOCK_STREAM, 0))) {
        (void)fprintf(stderr, "[ERROR] ::socket() failed %d\n", errno);

        return -1;
    }

    if (-1 == EW_SET_NONBLOCK(fd)) {
        (void)fprintf(stderr, "[ERROR] ::fcntl() failed %d\n", errno);
    }

    nopt = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                     (const void *)&nopt , sizeof(int)) < 0) {
        (void)fprintf(stderr, "[ERROR] ::setsockopt() failed %d\n", errno);
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = ::htons(1997);
    if (-1 == ::bind(fd,
                     reinterpret_cast<struct sockaddr *>(&addr),
                     sizeof(addr))) {
        RaiiFd free_socket_fd(fd);
        (void)fprintf(stderr, "[ERROR] ::bind() failed %d\n", errno);

        return -1;
    }

    if (-1 == ::listen(fd, 1)) {
        RaiiFd free_socket_fd(fd);
        (void)fprintf(stderr, "[ERROR] ::listen() failed %d\n", errno);

        return -1;
    }

    return fd;
}


int EwUtils::InitTcpListen6()
{
    int fd;
    int nopt;
    struct sockaddr_in6 addr;

    if (-1 == (fd = ::socket(AF_INET6, SOCK_STREAM, 0))) {
        (void)fprintf(stderr, "[ERROR] ::socket() failed %d\n", errno);

        return -1;
    }

    if (-1 == EW_SET_NONBLOCK(fd)) {
        (void)fprintf(stderr, "[ERROR] ::fcntl() failed %d\n", errno);
    }

    nopt = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                     (const void *)&nopt , sizeof(int)) < 0) {
        (void)fprintf(stderr, "[ERROR] ::setsockopt() failed %d\n", errno);
    }

    addr.sin6_family = AF_INET6;
    if (1 != ::inet_pton(AF_INET6, "::", &addr.sin6_addr)) {
        RaiiFd free_socket_fd(fd);
        return -1;
    }
    addr.sin6_port = ::htons(1997);
    if (-1 == ::bind(fd,
                     reinterpret_cast<struct sockaddr *>(&addr),
                     sizeof(addr))) {
        RaiiFd free_socket_fd(fd);
        (void)fprintf(stderr, "[ERROR] ::bind() failed %d\n", errno);

        return -1;
    }
    if (-1 == ::listen(fd, 1)) {
        RaiiFd free_socket_fd(fd);
        (void)fprintf(stderr, "[ERROR] ::listen() failed %d\n", errno);

        return -1;
    }

    return fd;
}


void EwUtils::ExitTcpListen(int socket_fd)
{
    RaiiFd free_socket_fd(socket_fd);

    return;
}
}


// ::easywork::Main_
namespace Main_ {
using ::std::list;

class Main : public DenyCopyable
{
public:
    explicit Main(void) {}
    virtual ~Main(void) {}
    virtual int Init(void)
    {
        return DenyCopyable::Init();
    }

public:
    int run(int argc, char *argv[], char *env[])
    {
        EwConf *ew_conf;
        ThreadList *thread_list;

        // 阻塞所有信号
        ::sigset_t block_all_sig;
        static_cast<void>(::sigfillset(&block_all_sig));
        static_cast<void>(::sigprocmask(SIG_SETMASK, &block_all_sig, NULL));

        // 初始化框架单例
        thread_list = new ThreadList();
        if (thread_list->Init()) {
            return EXIT_FAILURE;
        }
        ew_singleton_mng.HoldObjInst(SGT_THREAD_LIST, thread_list);

        ew_conf = new EwConf();
        if (thread_list->Init()) {
            return EXIT_FAILURE;
        }
        ew_singleton_mng.HoldObjInst(SGT_EWCONF, ew_conf);

        // 主线程初始化回调点
        if (init_master()) {
            return EXIT_FAILURE;
        }

        // 启动线程
        const list<ThreadSchema *> *thread_set = thread_list->GetThreadSet();
        if (thread_set->size()) {
            list<ThreadSchema *>::const_iterator it;

            for (it = thread_set->begin(); thread_set->end() != it; ++it) {
                for (int i = 0; i < (*it)->nthreads; ++i) {
                    (*it)->tg.create_thread((*it)->thread_proc);
                }
            }

            // 等待工作线程
            for (it = thread_set->begin(); thread_set->end() != it; ++it) {
                (*it)->tg.join_all();
            }
        }

        // 主线程清理回调点
        exit_master();

        return EXIT_SUCCESS;
    }
};
} // end of namespace Main_

} // end of namespace easywork

int main(int argc, char *argv[], char *env[])
{
    ::easywork::Main entity;

    return entity.run(argc, argv, env);
}
