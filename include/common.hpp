#ifndef __EASYWORK__COMMON_HPP__
#define __EASYWORK__COMMON_HPP__


#include <errno.h>

#include <cstdio>
#include <cstdlib>


#define NOP         static_cast<void>(0)

#define EW_SET_NONBLOCK(fd)    \
            fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK)


namespace easywork {

namespace DenyCopyable_ {
class DenyCopyable;
}
typedef DenyCopyable_::DenyCopyable DenyCopyable;

}


namespace easywork {

namespace DenyCopyable_ {
class DenyCopyable
{
protected:
    explicit DenyCopyable(void) {}
    virtual ~DenyCopyable(void) {}
    virtual int Init(void)
    {
        return 0;
    }

private:
    DenyCopyable(const DenyCopyable &);
    DenyCopyable const &operator =(const DenyCopyable &);
};
}

}
#endif // __EASYWORK__COMMON_HPP__
