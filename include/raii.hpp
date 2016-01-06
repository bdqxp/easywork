#ifndef __EASYWORK__RAII_HPP__
#define __EASYWORK__RAII_HPP__


#include <unistd.h>

#include "singleton_manager.hpp"


namespace easywork {

namespace RaiiFd_ {
class RaiiFd;
}
typedef RaiiFd_::RaiiFd RaiiFd;

}


namespace easywork {

namespace RaiiFd_ {
class RaiiFd : public DenyCopyable
{
public:
    explicit RaiiFd(int fd) : rsc(fd) {}
    virtual ~RaiiFd(void)
    {
        if (-1 == ::close(rsc)) {
            (void)fprintf(stderr, "[ERROR] ::close() failed %d\n", errno);
        }
    }

public:
    operator int(void)
    {
        return rsc;
    }

private:
    int rsc;
};
}

}
#endif // __EASYWORK__RAII_HPP__
