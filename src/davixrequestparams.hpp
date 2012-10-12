#pragma once
#ifndef DAVIX_REQUESTPARAMS_H
#define DAVIX_REQUESTPARAMS_H

#include "global_def.hpp"

namespace Davix {



class RequestParams
{
public:
    RequestParams();
    RequestParams(const RequestParams & params);

    virtual ~RequestParams();

    /**
      disable the certificate authority validity check for the https request
    */
    inline void setSSLCAcheck(bool chk){
        ssl_check = chk;
    }

    inline bool getSSLCACheck() const{
        return ssl_check;
    }



    /**
      authentification callback for right management
    */
    void setAuthentificationCallback(void * userdata, davix_auth_callback call);

    davix_auth_callback getAuthentificationCallbackFunction();
    void* getAuthentificationCallbackData();

    /// define the connexion established timeout
    void setConnexionTimeout(struct timespec* conn_timeout);

    const struct timespec * getConnexionTimeout()  const;


    /// define the maximum timeout for an operation
    void setOperationTimeout(struct timespec* cops_timeout);

    const struct timespec * getOperationTimeout()const;


    /// enable or disable transparent redirection support
    /// DEFAULT : enabled
    void setTransparentRedirectionSupport(bool redirection);

    bool getTransparentRedirectionSupport() const;

protected:
    struct timespec ops_timeout;
    struct timespec connexion_timeout;
    bool ssl_check;
    bool _redirection;

    // auth callback
    davix_auth_callback call;
    void* userdata;

    // dptr
};


} // namespace Davix

#endif // DAVIX_REQUESTPARAMS_H
