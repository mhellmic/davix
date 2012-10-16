#ifndef DAVIX_STAT_H
#define DAVIX_STAT_H

#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "davixcontext.hpp"
#include "abstractsessionfactory.hpp"
#include "httprequest.hpp"
#include <xmlpp/webdavpropparser.hpp>
#include <posix/davposix.hpp>

namespace Davix{


/**
  retrieve a webdav propfind stat request to the given url
    @param req : http request where to executethe query
    @return vector of characters of the query content
    @throw GLib::Error
  */
const std::vector<char> & req_webdav_propfind(HttpRequest* req); // throw Glib::Error

void fill_stat_from_fileproperties(struct stat* st, const  FileProperties & prop);

}

#endif // DAVIX_STAT_H
