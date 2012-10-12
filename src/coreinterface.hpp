#ifndef COREINTERFACE_HPP
#define COREINTERFACE_HPP

#include <ctime>
#include <cstring>
#include <sys/types.h>
#include <dirent.h>



#include <global_def.hpp>
#include <davix.h>
#include <httprequest.hpp>


#include <abstractsessionfactory.hpp>



namespace Davix {



class CoreInterface
{
public:
    CoreInterface();
    virtual ~CoreInterface(){}

    /**
      get the current registered session factory
    */
    virtual AbstractSessionFactory* getSessionFactory()=0;


    /**
      configure the default buffer size for internal transfer
    */
    virtual void set_buffer_size(const size_t value)=0;

};


typedef CoreInterface Composition;

} // namespace Davix

#endif // COREINTERFACE_HPP
