#include "test_stat_session.h"

#include <davixcontext.hpp>
#include <http_backend.hpp>
#include <glibmm/init.h>
#include <posix/davposix.hpp>

using namespace Davix;

int n_call=0;

int mycred_auth_callback(davix_auth_t token, const davix_auth_info_t* t, void* userdata, GError** err){
    GError * tmp_err=NULL;
    char login[2048];
    char passwd[2048];
    char *p,*auth_string =(char*) userdata;
    int ret ;
    bool login_password_auth_type = false;
    memset(login,'\0', sizeof(char)*2048);

    n_call++;
    if( (p = strchr(auth_string,':')) != NULL)
        login_password_auth_type = true;

    if(login_password_auth_type ){
        *((char*) mempcpy(login, auth_string, p-auth_string)) = '\0';
        strcpy(passwd, p+1 );
        ret = davix_set_login_passwd_auth(token, login, passwd, &tmp_err);

    }else{
        ret = davix_set_pkcs12_auth(token, (const char*)userdata, (const char*)NULL, &tmp_err);
    }

    if(ret != 0){
        fprintf(stderr, " FATAL authentification Error : %s", tmp_err->message);
        g_propagate_error(err, tmp_err);
    }
    return ret;
}


static void configure_grid_env(char * cert_path, RequestParams&  p){
    RequestParams params;
    params.setSSLCAcheck(false);
    params.setAuthentificationCallback(cert_path, &mycred_auth_callback);
}

int main(int argc, char** argv){
    if( argc < 3){
        std::cout << "Usage  : " << std::endl;
        std::cout <<"\t" << argv[0] << " [CERTIFICATE_PATH] [url] [url] ....  " << std::endl;
        return 0;
    }

    g_logger_set_globalfilter(G_LOG_LEVEL_MASK);

    try{
        RequestParams  p;
        std::auto_ptr<Context> c( new Context());
        DavPosix pos(c.get());
        if(argc > 2){
            configure_grid_env(argv[1], p);
        }


        for(int i =2 ; i< argc; ++i){
            struct stat st;
            pos.stat(argv[i], &st);

            std::cout << "stat success" << std::endl;
            std::cout << " atime : " << st.st_atime << std::endl;
            std::cout << " mtime : " << st.st_mtime << std::endl;
            std::cout << " ctime : " << st.st_ctime << std::endl;
            std::cout << " mode : 0" << std::oct << st.st_mode << std::endl;
            std::cout << " len : " << st.st_size << std::endl;
        }



        std::cout << "authentification callback has been called " << n_call << std::endl;
    }catch(Glib::Error & e){
        std::cout << " error occures : N°" << e.code() << "  " << e.what() << std::endl;
        return -1;
    }catch(std::exception & e){
        std::cout << " error occures :" << e.what() << std::endl;
        return -1;
    }
    return 0;
}

