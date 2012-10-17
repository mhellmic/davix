#include "davixstatusrequest.hpp"
#include <davix_types.h>

namespace Davix {


struct DavixErrorInternal{
    DavixErrorInternal(const std::string &scope, StatusCode::Code errCode, const std::string &errMsg){
        this->scope = scope;
        this->errMsg = errMsg;
        this->code = errCode;
    }

    std::string scope;
    StatusCode::Code code;
    std::string errMsg;
};


DavixError::DavixError(const std::string &scope, StatusCode::Code errCode, const std::string &errMsg){
    d_ptr = new DavixErrorInternal(scope, errCode, errMsg);
}


DavixError::~DavixError(){
    delete d_ptr;
}


const std::string & DavixError::getErrMsg() const{
    return d_ptr->errMsg;
}

void DavixError::setErrMsg(const std::string &msg){
    d_ptr->errMsg = msg;
}

void DavixError::setStatus(const StatusCode::Code c){
    d_ptr->code = c;
}

StatusCode::Code DavixError::getStatus() const{
    return d_ptr->code;
}


void DavixError::setupError(DavixError **err, const std::string &scope, StatusCode::Code errCode, const std::string &errMsg){
    if(err){
        if(*err){
            // error msg
        }
        *err = new DavixError(scope, errCode, errMsg);
    }
}




DAVIX_C_DECL_BEGIN

using namespace Davix;

///  @brief clear a davix error object and release its memory, and set the error pointer to NULL
///
void davix_error_clear(davix_error_t* ptr_err){
    DavixError::clearError((DavixError**) ptr_err);
}

/// @brief create a new davix error object
///
void davix_error_setup(davix_error_t* ptr_err, const char* scope, int status_code, const char* msg){
    DavixError::setupError((DavixError**) ptr_err, scope, (Davix::StatusCode::Code)status_code, msg);
}


const char* davix_error_msg(davix_error_t err){
    return ((DavixError*) err)->getErrMsg().c_str();
}

int davix_error_code(davix_error_t err){
    return ((DavixError*) err)->getStatus();
}

const char* davix_error_scope(davix_error_t err){
    return NULL; // TODO
}


DAVIX_C_DECL_END

} // namespace Davix
