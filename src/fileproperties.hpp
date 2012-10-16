#ifndef DAVIX_FILEPROPERTIES_H
#define DAVIX_FILEPROPERTIES_H

#include "global_def.hpp"
#include <glibmm.h>

namespace Davix {

struct FileProperties
{

    Glib::ustring  filename;
    unsigned int  req_status; /* status code of the request associated ( ex: http 200) */

    nlink_t   nlink;
    uid_t     uid;    /* unix user id */
    gid_t     gid;   /* unix group id */
    off_t     size;  /* total size, in bytes */
    mode_t    mode;

    time_t    atime;   /* time of last access */
    time_t    mtime;   /* time of last modification */
    time_t    ctime;   /* time of last status change */

    inline void clear(){
        nlink = req_status =  gid = uid = size =0;
        mode = atime = mtime = ctime = 0;
    }

};

} // namespace Davix

#endif // DAVIX_FILEPROPERTIES_H
