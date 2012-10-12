#include "davixcontext.h"

#include <posixgate.h>
#include <httpgate.h>
#include <contextinternal.h>
#include <glibmm.h>

namespace Davix{

Context::Context() : _intern(new ContextInternal())
{
}

Context::Context(const Context &c) : ContextConfig(c){
    this->_intern = c._intern;
}

Context::~Context(){

}

Context* Context::clone(){

    return new Context(*this);
}


}
