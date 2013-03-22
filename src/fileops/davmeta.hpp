#ifndef DAVMETA_HPP
#define DAVMETA_HPP

#include <davixcontext.hpp>
#include <params/davixrequestparams.hpp>
#include <request/httpcachetoken.hpp>
#include <file/davfile.hpp>

namespace Davix{

namespace Meta{


// get all reps from webdav queries
dav_ssize_t getAllReplicas(Context & c, const Uri & r,
                              const RequestParams & params, ReplicaVec & vec, DavixError** err);

dav_ssize_t posixStat(Context & c, const Uri & url, const RequestParams * _params,
                      struct stat* st, HttpCacheToken** token_ptr,
                      DavixError** err);

/*
  retrieve a webdav propfind stat request to the given url
    @param req : http request where to executethe query
    @return vector of characters of the query content
  */
const char* req_webdav_propfind(HttpRequest* req, DavixError** err);


} // Meta

} // Davix




#endif // DAVMETA_HPP
