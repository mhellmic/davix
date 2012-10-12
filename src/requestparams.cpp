#include "requestparams.hpp"

namespace Davix {

RequestParams::RequestParams()
{
    connexion_timeout = DAVIX_DEFAULT_CONN_TIMEOUT;
    ops_timeout = DAVIX_DEFAULT_OPS_TIMEOUT;
    call =NULL;
    userdata = NULL;
}

RequestParams::RequestParams(const RequestParams& params){
    call = params.call;
    userdata = params.userdata;
    connexion_timeout = params.connexion_timeout;
    ops_timeout = params.ops_timeout;

}

RequestParams::~RequestParams(){

}

//
void RequestParams::setSSLCAcheck(bool chk){
    ssl_check = chk;
}

//
void RequestParams::setAuthentificationCallback(void * _userdata, davix_auth_callback _call){
    call = _call;
    userdata = _userdata;
}

//
void RequestParams::setConnexionTimeout(unsigned long timeout){ // throw nothing
    connexion_timeout = timeout;
}

//
void RequestParams::setOperationTimeout(unsigned long timeout){ // throw nothing
    ops_timeout = timeout;
}


} // namespace Davix
