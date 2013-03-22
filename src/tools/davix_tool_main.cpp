

#include <davix.hpp>
#include <tools/davix_tool_params.hpp>
#include <tools/davix_tool_util.hpp>
#include <cstdio>


// @author : Devresse Adrien
// main file for davix low level cmd line tool


using namespace Davix;
using namespace std;

#define READ_BLOCK_SIZE 4096


const std::string scope_main = "Davix::Tools::davix";


static void configure_req(HttpRequest& req, Tool::OptParams & opts){
    req.setParameters(opts.params);
    if(opts.req_type.empty() == false)
        req.setRequestMethod(opts.req_type);

    for(Tool::HeaderVec::iterator it = opts.header_args.begin(); it < opts.header_args.end(); it++){
            req.addHeaderField(it->first, it->second);
    }

    req.setRequestBodyString(opts.req_content);

}

static std::string help_msg(){
    return Tool::get_base_description_options() +
            Tool::get_common_options() +
           "  Request Options: \n"
           "\t--data                    Content of the request\n"
           "\t--header, -H:             Add a header field to the request (ex: \"Depth: 1\") \n"
           "\t--request, -X:            Request operation to use (ex : GET, PUT, PROPFIND, etc..)\n"
           "\n"
                       ;
}


static int read_stream(HttpRequest* req, FILE* fstream, DavixError** err){
    char buffer [READ_BLOCK_SIZE];

    ssize_t s_read=1;
    if(req->beginRequest(err) <0 )
        return -1;
    do{
         if( (s_read = req->readBlock(buffer, READ_BLOCK_SIZE, err)) <0)
             return -1;
         fwrite(buffer, s_read, 1, fstream);
   }while(s_read > 0);
   return req->endRequest(err);
}

int main(int argc, char** argv){
    int retcode=-1;
    Tool::OptParams opts;
    DavixError* tmp_err=NULL;
    opts.help_msg = help_msg();
    FILE* fstream;

    if( (retcode= Tool::parse_davix_options(argc, argv, opts, &tmp_err)) ==0){
        Context c;
        if( (fstream= Tool::configure_fstream(opts, scope_main, &tmp_err)) != NULL
            && (retcode = Tool::setup_credential(opts, &tmp_err)) == 0){

            HttpRequest req(c, opts.vec_arg[0], &tmp_err);
            if( tmp_err == NULL){
                configure_req(req, opts);
                retcode= read_stream(&req, fstream, &tmp_err);
                fflush(stdout);
            }else{
                retcode =-1;
            }
        }
    }
    Tool::err_display(&tmp_err);
    return retcode;
}







