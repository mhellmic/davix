#ifndef DAVIX_HTTPCACHETOKEN_HPP
#define DAVIX_HTTPCACHETOKEN_HPP

#include <vector>
#include <davix_types.h>
#include <davixuri.hpp>

#ifndef __DAVIX_INSIDE__
#error "Only davix.h or davix.hpp should be included."
#endif

/**
  @file httpcachetoken.hpp
  @author Devresse Adrien

  @brief Cache control feature of davix
 */




namespace Davix {

struct HttpCacheTokenInternal;

/// @class HttpCacheToken
/// @brief State of the cache of a past HTTPRequest
/// transmit a \ref HttpCacheToken object enable speed optimizations
///
class DAVIX_EXPORT HttpCacheToken
{
public:
    HttpCacheToken();
    HttpCacheToken(const HttpCacheToken & orig);
    virtual ~HttpCacheToken();

    HttpCacheToken & operator=(const HttpCacheToken &);

    const Uri & getrequestUri() const;

    /// @brief get the final redirection Uri associated with the request Uri
    const Uri & getCachedRedirection() const;
private:
    HttpCacheTokenInternal* d_ptr;

    friend struct HttpCacheTokenAccessor;
};


} // namespace Davix

#endif // DAVIX_HTTPREQUEST_H
