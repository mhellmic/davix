#include <davix_internal.hpp>
#include "metalinkops.hpp"

#include <string_utils/stringutils.hpp>
#include <utils/davix_logger_internal.hpp>
#include <xml/metalinkparser.hpp>
#include <base64/base64.hpp>


namespace Davix{


using namespace StrUtil;
using namespace boost;

typedef function< dav_ssize_t (IOChainContext &)> FuncIO;
typedef function< StatInfo & (IOChainContext &) > FuncStatInfo;


static bool metalink_support_disabled=false;
static once_flag metalink_once = BOOST_ONCE_INIT;

void metalink_check(){
    metalink_support_disabled = (getenv("DAVIX_DISABLE_METALINK") == NULL);
}

static bool isMetalinkEnabled(const RequestParams* params){
    call_once(metalink_check, metalink_once);
    return !( (params != NULL && params->getMetalinkMode() == MetalinkMode::Disable) || metalink_support_disabled);
}


template<class Executor, class ReturnType>
ReturnType metalinkTryReplicas(HttpIOChain & chain, IOChainContext & io_context, Executor fun){
    std::vector<File> replicas;
    chain.getReplicas(io_context, replicas);
    for(std::vector<File>::iterator it = replicas.begin();it != replicas.end(); ++it){
        try{
            IOChainContext internal_context(io_context._context, it->getUri(), io_context._reqparams);
            return fun(internal_context);
        }catch(DavixException & replica_error){
            DAVIX_LOG(DAVIX_LOG_VERBOSE, "Fail access to replica %s: %s", it->getUri().getString().c_str(), replica_error.what());
        }catch(...){
            DAVIX_LOG(DAVIX_LOG_VERBOSE, "Fail access to replica: Unknown Error");
        }
    }
    throw DavixException(davix_scope_io_buff(), StatusCode::InvalidServerResponse, "Impossible to access any of the replicas with success");
}


template<class Executor, class ReturnType>
ReturnType metalinkExecutor(HttpIOChain & chain, IOChainContext & io_context, Executor fun){
    // if disabled, do nothing
    if(isMetalinkEnabled(io_context._reqparams)){
        return fun(io_context);
    }

    try{
        // Execute operation
        return fun(io_context);
    }catch(DavixException & e){
        DAVIX_LOG(DAVIX_LOG_VERBOSE, "Failure: Impossible to execute operation on %s, error %s", io_context._uri.getString().c_str(), e.what());
        DAVIX_LOG(DAVIX_LOG_VERBOSE, " Try to Recover with Metalink...");

        try{
            return metalinkTryReplicas<Executor, ReturnType>(chain, io_context, fun);
        }catch(DavixException & metalink_error){
            DAVIX_LOG(DAVIX_LOG_VERBOSE, "Impossible to Recover with Metalink: %s", metalink_error.what());
        }catch(...){
            DAVIX_LOG(DAVIX_LOG_VERBOSE, "Impossible to Recover with Metalink: Unknown Error");
        }
        throw e;
    }
}



int davix_metalink_header_parser(const std::string & header_key, const std::string & header_value,
                                 const Uri & u_original,
                                 Uri & metalink){
    DAVIX_TRACE("Parse headers for metalink %s %s", header_key.c_str(), header_value.c_str());

    if(compare_ncase(header_key, "Link") ==0 && header_value.find("application/metalink") != std::string::npos){
        std::string::const_iterator it1, it2;
        if( ( it1 = std::find(header_value.begin(), header_value.end(), '<')) != header_value.end()
                && ( it2 = std::find(it1, header_value.end(), '>')) != header_value.end()){
            std::string metalink_url(it1+1, it2);
            metalink =  Uri::fromRelativePath(u_original, trim(metalink_url));
            if(metalink.getStatus() == StatusCode::OK){
                DAVIX_TRACE("Valid metalink URI found %s", metalink.getString().c_str());
                return 1;
            }

        }
    }
    return 0;
}

bool davix_metalink_header_content_type(const std::string & header_key, const std::string & header_value){
    return (compare_ncase(header_key, "Content-type") ==0 &&  header_value.find("application/metalink") !=std::string::npos);
}

int davix_get_metalink_url( Context & c, const Uri & uri,
                            const RequestParams & _params, Uri & metalink){
    DavixError* tmp_err = NULL;
    RequestParams params(_params);
    // don't follow redirect, we need headers
    params.setTransparentRedirectionSupport(false);
    HeadRequest req(c, uri, &tmp_err);
    req.setParameters(params);
    req.addHeaderField("Accept", "application/metalink4+xml");


    DAVIX_TRACE("Executing head query to %s for Metalink file", uri.getString().c_str());
    if(tmp_err != NULL || (req.executeRequest(&tmp_err) <0))
        throw DavixException(davix_scope_meta(), tmp_err->getStatus(), tmp_err->getErrMsg());

    HeaderVec headers;
    req.getAnswerHeaders(headers);
    for(HeaderVec::iterator it = headers.begin(); it != headers.end(); it++){
        if( davix_metalink_header_parser(it->first, it->second, uri, metalink) > 0)
            return 1;

        if(davix_metalink_header_content_type(it->first, it->second)){
            // is a metalink content type, get it
            metalink = uri;
            return 1;
        }
    }




    return 0;
}


int davix_file_get_metalink_to_vfile(Context & c, const Uri & metalink_uri,
                                     const RequestParams & _params, std::vector<DavFile> & vec){
    DavixError * tmp_err=NULL;
    GetRequest req(c, metalink_uri, &tmp_err);
    MetalinkParser parser(c, vec);

    req.setParameters(_params);
    req.addHeaderField("Accept", "application/metalink4+xml");

    DAVIX_TRACE("Executing query for %s Metalink content", metalink_uri.getString().c_str());
    if(tmp_err != NULL || (req.beginRequest(&tmp_err) <0) )
        throw DavixException(davix_scope_meta(), tmp_err->getStatus(), tmp_err->getErrMsg());
    if(httpcodeIsValid(req.getRequestCode()) == false){
        std::ostringstream ss;
        ss << "Unable to get Metalink file, error HTTP " << req.getRequestCode();
        throw DavixException(davix_scope_meta(), StatusCode::InvalidServerResponse, ss.str());
    }

    dav_ssize_t read_size;
    do{
        char buffer[2049];
        buffer[2048] = '\0';
        if( (read_size = req.readSegment(buffer, 2048, &tmp_err)) < 0)
            throw DavixException(davix_scope_meta(), tmp_err->getStatus(), tmp_err->getErrMsg());
        parser.parseChuck(buffer, read_size);
    }while(read_size > 0);

    req.endRequest(NULL);
    return vec.size();
}

void davix_file_get_all_replicas_metalink( Context & c, const Uri & uri,
                                 const RequestParams & _params, std::vector<DavFile> & vec){
    Uri metalink;
    if(davix_get_metalink_url(c,  uri,
                              _params, metalink) > 0
            && davix_file_get_metalink_to_vfile(c, metalink,_params, vec) > 0){
        return;

    }
    throw DavixException(davix_scope_meta(), StatusCode::OperationNonSupported, "Server does not support Metalink standard");
}


MetalinkOps::MetalinkOps()
{

}

MetalinkOps::~MetalinkOps(){

}



std::vector<File> & MetalinkOps::getReplicas(IOChainContext & iocontext, std::vector<File> &vec){
    davix_file_get_all_replicas_metalink(iocontext._context, iocontext._uri, iocontext._reqparams, vec);
    return vec;
}

StatInfo & MetalinkOps::statInfo(IOChainContext &iocontext, StatInfo &st_info){
    FuncStatInfo func(bind(&HttpIOChain::statInfo, _next.get(), _1, ref(st_info)));
    return metalinkExecutor<FuncStatInfo, StatInfo &>(*this, iocontext, func);
}

dav_ssize_t MetalinkOps::read(IOChainContext &iocontext, void *buf, dav_size_t count){
    FuncIO func(bind(&HttpIOChain::read, _next.get(),_1, buf, count));
    return metalinkExecutor<FuncIO, dav_ssize_t>(*this, iocontext, func);
}

dav_ssize_t MetalinkOps::pread(IOChainContext &iocontext, void *buf, dav_size_t count, dav_off_t offset){

    FuncIO func(bind(&HttpIOChain::pread, _next.get(),_1, buf, count, offset));
    return metalinkExecutor<FuncIO, dav_ssize_t>(*this, iocontext, func);
}


dav_ssize_t MetalinkOps::preadVec(IOChainContext & iocontext, const DavIOVecInput * input_vec,
                          DavIOVecOuput * output_vec,
                          const dav_size_t count_vec){

    FuncIO func(bind(&HttpIOChain::preadVec, _next.get(),_1, input_vec, output_vec, count_vec));
    return metalinkExecutor<FuncIO, dav_ssize_t>(*this, iocontext, func);
}

// read to fd Metalink manager
dav_ssize_t MetalinkOps::readToFd(IOChainContext & iocontext, int fd, dav_size_t size){
    FuncIO func(bind(&HttpIOChain::readToFd, _next.get(), _1, fd, size));
    return metalinkExecutor<FuncIO, dav_ssize_t>(*this, iocontext, func);
}

} // namespace Davix

