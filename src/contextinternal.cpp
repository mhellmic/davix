#include "contextinternal.h"

#include <neon/neonsessionfactory.hpp>
namespace Davix {

using namespace Glib;

Core::Core(AbstractSessionFactory* fsess) : _fsess(fsess)
{
    _s_buff = 65536;
    _timeout = 300;
}


AbstractSessionFactory* Core::getSessionFactory(){
    return _fsess.get();
}

void Core::set_buffer_size(const size_t value){
    _s_buff = value;
}


ContextInternal::ContextInternal() : _fsess(new NEONSessionFactory())
{
}

NEONSessionFactory* ContextInternal::getSessionFactory(){
    return _fsess.get();
}

} // namespace Davix
