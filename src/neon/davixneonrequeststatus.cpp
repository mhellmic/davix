#include "davixneonrequeststatus.h"
#include <neon/neongenericrequest.h>

namespace Davix {

NeonRequestResult::NeonRequestResult(Context* context, NeonGenericRequest* request) : DavStatusRequest(context, request)
{

}

} // namespace Davix
