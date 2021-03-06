/*
 * This File is part of Davix, The IO library for HTTP based protocols
 * Copyright (C) 2013  Adrien Devresse <adrien.devresse@cern.ch>, CERN
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
*/

#include <davix_internal.hpp>

#include <utils/davix_logger_internal.hpp>
#include <status/davixstatusrequest.hpp>
#include <fileops/fileutils.hpp>
#include <fileops/davmeta.hpp>
#include <fileops/httpiovec.hpp>
#include <fileops/metalinkops.hpp>
#include <xml/davpropxmlparser.hpp>
#include <fileops/iobuffmap.hpp>
#include <string_utils/stringutils.hpp>
#include <posix/davposix.hpp>

using namespace StrUtil;



namespace Davix{



static HttpIOChain & getIOChain(HttpIOChain & chain){
    chain.add(new MetalinkOps())->add(new HttpMetaOps())->add(new HttpIOBuffer())->add(new HttpIO())->add(new HttpIOVecOps());
    return chain;
}

static IOChainContext  getIOContext( Context & context, const Uri & uri, const RequestParams* params){
        return IOChainContext(context, uri, params);
}

}

static const std::string simple_listing("<propfind xmlns=\"DAV:\"><prop><resourcetype><collection/></resourcetype></prop></propfind>");

static const std::string stat_listing("<?xml version=\"1.0\" encoding=\"utf-8\" ?><D:propfind xmlns:D=\"DAV:\" xmlns:L=\"LCGDM:\"><D:prop>"
                                      "<D:displayname/><D:getlastmodified/><D:creationdate/><D:getcontentlength/>"
                                      "<D:resourcetype><D:collection/></D:resourcetype><L:mode/>"
                                      "</D:prop>"
                                      "</D:propfind>");

struct Davix_dir_handle{

    Davix_dir_handle(Davix::HttpRequest* _req, Davix::DavPropXMLParser * _parser):
        request(_req),
        parser(_parser),
        dir_info((struct dirent*) calloc(1,sizeof(struct dirent) + NAME_MAX +1)),
        dir_offset(0){

    }

   ~Davix_dir_handle(){
        delete request;
        delete parser;
        free(dir_info);
    }

   Davix::HttpRequest* request;
   Davix::DavPropXMLParser * parser;
   struct dirent* dir_info;
   off_t dir_offset;

private:
   Davix_dir_handle(const Davix_dir_handle & );
   Davix_dir_handle & operator=(const Davix_dir_handle & );
};



struct Davix_fd{
    Davix_fd(Davix::Context & context, const Davix::Uri & uri, const Davix::RequestParams * params) : _uri(uri), _params(params),
        io_handler(), io_context(Davix::getIOContext(context, _uri, &_params)){
        Davix::getIOChain(io_handler);
    }
    virtual ~Davix_fd(){
        try{
            io_handler.resetIO(io_context);
        }catch(Davix::DavixException & e){
            DAVIX_LOG(DAVIX_LOG_VERBOSE, "Error when closed file descriptor, possibly file corrupted %s", e.what());
        }
    }

    Davix::Uri _uri;
    Davix::RequestParams _params;
    Davix::HttpIOChain io_handler;
    Davix::IOChainContext io_context;
};


namespace Davix {

DavPosix::DavPosix(Context* _context) :
    context(_context),
    _timeout(180),
    _s_buff(2048),
    d_ptr(NULL)

{
    (void ) d_ptr; // silence warning
}

DavPosix::~DavPosix(){

}




static void fill_dirent_from_filestat(struct dirent * d, const FileProperties & f){
    copy_std_string_to_buff(d->d_name, NAME_MAX, f.filename);
    if (S_ISDIR(f.mode))
        d->d_type = DT_DIR;
    else if (S_ISLNK(f.mode))
        d->d_type = DT_LNK;
    else
        d->d_type = DT_REG;
}

dav_ssize_t incremental_propfind_listdir_parsing(HttpRequest* req, DavPropXMLParser * parser, dav_size_t s_buff, const char* scope, DavixError** err){
  //  std::cout << "time 1 pre-fecth" << time(NULL) << std::endl;
    DavixError* tmp_err=NULL;

    char buffer[s_buff+1];
    const dav_ssize_t ret = req->readSegment(buffer, s_buff, &tmp_err);
    if(ret >= 0){
        buffer[ret]= '\0';
        DAVIX_DEBUG("chunk parse : result content : %s", buffer);
        TRY_DAVIX{
            parser->parseChuck(buffer, ret);
        }CATCH_DAVIX(&tmp_err)
        if(tmp_err != NULL)
            return -1;
    }

    //std::cout << "time 3 post-parse" << time(NULL) << std::endl;
    DavixError::propagateError(err, tmp_err);
    return ret;
}




void configure_req_for_listdir(HttpRequest* req){
    req->addHeaderField("Depth","1");
}

DAVIX_DIR* DavPosix::internal_opendirpp(const RequestParams* _params, const char * scope, const std::string & body, const std::string & url, DavixError** err ){
    dav_ssize_t s_resu;
    DAVIX_DIR* res = NULL;
    int ret =-1;
    DavixError* tmp_err=NULL;
    RequestParams params(_params);
    PropfindRequest* http_req = new PropfindRequest(*context, url, &tmp_err);

    // create a new connexion + parser for this opendir
    if(tmp_err == NULL){
        configure_req_for_listdir(http_req);
         res=  new DAVIX_DIR(http_req, new DavPropXMLParser());
        time_t timestamp_timeout = time(NULL) + _timeout;

        http_req->setParameters(params);
        DavPropXMLParser* parser = res->parser;
        // setup the handle for simple listing only
        http_req->setRequestBody(body);


        if( (ret = http_req->beginRequest(&tmp_err)) == 0){ // start req


            if( ( ret = davixRequestToFileStatus(http_req,davix_scope_directory_listing_str(), &tmp_err)) == 0){

                    size_t prop_size = 0;
                    do{ // parse the begining of the request until the first property -> directory property
                       if( (s_resu = incremental_propfind_listdir_parsing(http_req, parser, this->_s_buff, scope, &tmp_err)) <0)
                           break;

                       prop_size = parser->getProperties().size();
                       if(s_resu < _s_buff && prop_size <1){ // verify request status : if req done + no data -> error
                           DavixError::setupError(&tmp_err, davix_scope_directory_listing_str(), StatusCode::WebDavPropertiesParsingError, "bad server answer, not a valid WebDav PROPFIND answer");
                           break;
                       }
                       if(timestamp_timeout < time(NULL)){
                           DavixError::setupError(&tmp_err, davix_scope_directory_listing_str(), StatusCode::OperationTimeout, "operation timeout triggered while directory listing");
                           break;
                       }

                    }while( prop_size < 1); // leave is end of req & no data

                    if(!tmp_err){
                        if( S_ISDIR(parser->getProperties().at(0).mode) == false){
                             DavixError::setupError(&tmp_err, davix_scope_directory_listing_str(), StatusCode::IsNotADirectory, url + " is not a collection or a directory, impossible to list");
                        }else{
                            parser->getProperties().pop_front(); // suppress the parent directory infos...
                        }
                    }
            }
        }
    }

    if(tmp_err){
        DavixError::propagateError(err, tmp_err);
        delete res;
        return 0;
    }
    return (DAVIX_DIR*) res;
}


DAVIX_DIR* DavPosix::opendir(const RequestParams* params, const std::string &url, DavixError** err){

    DAVIX_DEBUG(" -> davix_opendir");
    DAVIX_DIR* r  = NULL;

    TRY_DAVIX{
        r =  internal_opendirpp(params, "Core::opendir",simple_listing, url, err);
    }CATCH_DAVIX(err)

    DAVIX_DEBUG(" <- davix_opendir");
    return (DAVIX_DIR*) r;
}

DAVIX_DIR* DavPosix::opendirpp(const RequestParams* params, const std::string &url, DavixError** err){

    DAVIX_DEBUG(" -> davix_opendirpp");
    DAVIX_DIR* r  = NULL;

    TRY_DAVIX{
        r =  internal_opendirpp(params, "Core::opendir",stat_listing, url, err);
    }CATCH_DAVIX(err)

    DAVIX_DEBUG(" <- davix_opendirpp");
    return (DAVIX_DIR*) r;
}


struct dirent* DavPosix::readdir(DAVIX_DIR * d, DavixError** err){
    DAVIX_DEBUG(" -> davix_readdir");
    DavixError* tmp_err=NULL;
    DAVIX_DIR* handle = static_cast<DAVIX_DIR*>(d);

    TRY_DAVIX{

        if( d==NULL){
            throw DavixException(davix_scope_directory_listing_str(), StatusCode::InvalidFileHandle,  "Invalid file descriptor for DAVIX_DIR*");
        }

        HttpRequest *req = handle->request; // setup env again
        DavPropXMLParser* parser = handle->parser;
        off_t read_offset = handle->dir_offset+1;
        size_t prop_size = parser->getProperties().size();
        ssize_t s_resu = _s_buff;

        while( prop_size == 0
              && s_resu > 0){ // request not complete and current data too smalls
            // continue the parsing until one more result
           if( (s_resu = incremental_propfind_listdir_parsing(req, parser, this->_s_buff, "Davix::readdir", &tmp_err)) <0)
               break;

           prop_size = parser->getProperties().size();
        }

        if(!tmp_err){
            if(prop_size == 0) // end of the request, end of the story
                return NULL;
            fill_dirent_from_filestat(handle->dir_info, parser->getProperties().front());
            handle->dir_offset = read_offset;
            parser->getProperties().pop_front(); // clean the current element
            DAVIX_DEBUG(" <- davix_readdir");
            return handle->dir_info;
        }
    }CATCH_DAVIX(&tmp_err)

    DavixError::propagateError(err, tmp_err);
    return NULL;
}

struct dirent* DavPosix::readdirpp(DAVIX_DIR * d, struct stat *st, DavixError** err){
    DAVIX_DEBUG(" -> davix_readdirpp");
    DavixError* tmp_err=NULL;
    struct dirent* ret = NULL;
    DAVIX_DIR* handle = static_cast<DAVIX_DIR*>(d);

    TRY_DAVIX{

        if( d==NULL){
            throw DavixException(davix_scope_directory_listing_str(), StatusCode::InvalidFileHandle,  "Invalid file descriptor for DAVIX_DIR*");
        }


        HttpRequest* req = handle->request; // setup env again
        DavPropXMLParser* parser = handle->parser;
        off_t read_offset = handle->dir_offset+1;
        size_t prop_size = parser->getProperties().size();
        ssize_t s_resu = _s_buff;

        while( prop_size == 0
              && s_resu > 0){ // request not complete and current data too smalls
            // continue the parsing until one more result
           if( (s_resu = incremental_propfind_listdir_parsing(req, parser, this->_s_buff, "Davix::readdirpp", &tmp_err)) <0)
               break;

           prop_size = parser->getProperties().size();
        }

        if(!tmp_err){
            if(prop_size == 0){
                ret= NULL; // end of the request, end of the story
            }else{
                fill_dirent_from_filestat(handle->dir_info, parser->getProperties().front());
                handle->dir_offset = read_offset;
                fill_stat_from_fileproperties(st, parser->getProperties().front());
                parser->getProperties().pop_front(); // clean the current element
                ret= handle->dir_info;
            }
            DAVIX_DEBUG(" <- davix_readdirpp");
        }

    }CATCH_DAVIX(&tmp_err)

    DavixError::propagateError(err, tmp_err);
    return ret;
}


int DavPosix::closedirpp(DAVIX_DIR * d, DavixError** err){
    int ret =-1;

    TRY_DAVIX{
        if( d==NULL){
            throw DavixException(davix_scope_directory_listing_str(), Davix::StatusCode::InvalidFileHandle, "Invalid file descriptor for DAVIX_DIR*");
       }else{
            delete (static_cast<DAVIX_DIR*>(d));
            ret =0;
        }

    }CATCH_DAVIX(err)

    return ret;
}

int DavPosix::closedir(DAVIX_DIR * d, DavixError** err){
    return closedirpp(d,err);
}

void DavPosix::fadvise(DAVIX_FD *fd, dav_off_t offset, dav_size_t len, advise_t advise){
    try{
        if( fd==NULL)
            return;
        fd->io_handler.prefetchInfo(fd->io_context, offset, len, advise);

    }catch(DavixException & e){
        DAVIX_LOG(DAVIX_LOG_WARNING, "[DavPosix::fadvise] error %s", e.what());
    }catch(...){
        DAVIX_LOG(DAVIX_LOG_WARNING, "[DavPosix::fadvise] Unknown error, aborted");
    }
}


////////////////////////////////////////////////////
//////////////////// Davix POSIX meta ops
/////////////////////////////////////////////////////


int DavPosix::mkdir(const RequestParams * _params, const std::string &url, mode_t right, DavixError** err){
    DAVIX_DEBUG(" -> davix_mkdir");
    int ret=-1;

    TRY_DAVIX{
        Uri uri(url);
        HttpIOChain chain;
        IOChainContext io_context = getIOContext(*context, uri, _params);

        getIOChain(chain).makeCollection(io_context);
        ret = 0;
    }CATCH_DAVIX(err)

    DAVIX_DEBUG(" davix_mkdir <-");
    return ret;
}


int DavPosix::stat(const RequestParams * params, const std::string & url, struct stat* st, DavixError** err){
    DAVIX_DEBUG(" -> davix_stat");
    DavixError* tmp_err=NULL;
    int ret = -1;

    TRY_DAVIX{
        File f(*context, url);
        ret = f.stat(params, st, err);
    }CATCH_DAVIX(&tmp_err)

    DAVIX_DEBUG(" davix_stat <-");
    DavixError::propagatePrefixedError(err, tmp_err, "stat ops : ");
    return ret;

}

int davix_remove_posix(DavPosix & p, Context* context, const RequestParams * params, const std::string & url, bool directory, DavixError** err){
    DavixError* tmp_err = NULL;
    int ret = -1;
    Uri uri(url);

    TRY_DAVIX{
        if(params && params->getProtocol() == RequestProtocol::Http){ // pure protocol http : ignore posix semantic, execute a simple delete
            HttpIOChain chain;
            IOChainContext io_context = getIOContext(*context, uri, params);
            getIOChain(chain).deleteResource(io_context);
            ret = 0;
        }else{
            // full posix semantic support
            HttpIOChain chain;
            IOChainContext io_context = getIOContext(*context, uri, params);
            struct StatInfo infos;

            getIOChain(chain).statInfo(io_context, infos);

            if( S_ISDIR(infos.mode)){ // directory : impossible to delete if not empty
                if(directory == true){
                    chain.deleteResource(io_context);
                    ret =0;
                }else{
                    ret = -1;
                    std::ostringstream ss;
                    ss << " " << uri << " is not a directory, impossible to unlink"<< std::endl;
                    throw DavixException(davix_scope_davOps_str(), StatusCode::IsADirectory, ss.str());
                }
            }else{ // file, rock & roll
                if(directory == false){
                    chain.deleteResource(io_context);
                    ret =0;
                }else{
                    ret = -1;
                    std::ostringstream ss;
                    ss << " " << uri << " is not a directory, impossible to rmdir"<< std::endl;
                    throw DavixError(davix_scope_davOps_str(), StatusCode::IsNotADirectory, ss.str());
                }
            }
        }
    }CATCH_DAVIX(&tmp_err)

    DavixError::propagateError(err, tmp_err);
    return ret;

}


int DavPosix::unlink(const RequestParams * params, const std::string &uri, DavixError** err){
    DAVIX_DEBUG(" -> davix_unlink");
    int ret=-1;
    DavixError* tmp_err=NULL;

    TRY_DAVIX{
        ret = davix_remove_posix(*this, context, params, uri, false, &tmp_err);
    }CATCH_DAVIX(&tmp_err)

    DAVIX_DEBUG(" davix_unlink <-");
    DavixError::propagatePrefixedError(err, tmp_err, "DavPosix::unlink ");
    return ret;
}


int DavPosix::rmdir(const RequestParams * params, const std::string &uri, DavixError** err){
    DAVIX_DEBUG(" -> davix_rmdir");
    int ret=-1;
    DavixError* tmp_err=NULL;

    TRY_DAVIX{
        ret = davix_remove_posix(*this, context, params, uri, true, &tmp_err);
    }CATCH_DAVIX(&tmp_err)

    DAVIX_DEBUG(" davix_rmdir <-");
    DavixError::propagatePrefixedError(err, tmp_err, "DavPosix::rmdir ");
    return ret;
}



////////////////////////////////////////////////////
//////////////////// Davix POSIX I/O
/////////////////////////////////////////////////////



inline int davix_check_rw_fd(DAVIX_FD* fd, DavixError** err){
    if(fd == NULL){
        DavixError::setupError(err, davix_scope_http_request(),StatusCode::InvalidFileHandle, "Invalid Davix file descriptor");
        return -1;
    }
    return 0;
}


DAVIX_FD* DavPosix::open(const RequestParams * _params, const std::string & url, int flags, DavixError** err){
    DAVIX_DEBUG(" -> davix_open");
    DavixError* tmp_err=NULL;
    Davix_fd* fd = NULL;


    TRY_DAVIX{
        Uri uri(url);

        if(uri.getStatus() != StatusCode::OK){
            throw DavixException(davix_scope_http_request(), uri.getStatus(), " Uri invalid in Davix::Open");
        }
        fd = new Davix_fd(*context, uri, _params);
        fd->io_handler.open(fd->io_context, flags);
    }CATCH_DAVIX(&tmp_err)

    if(tmp_err){
        DavixError::propagateError(err, tmp_err);
        delete fd;
        fd= NULL;
    }
    DAVIX_DEBUG(" davix_open <-");
    return fd;
}


ssize_t DavPosix::read(DAVIX_FD* fd, void* buf, size_t count, Davix::DavixError** err){
    DAVIX_DEBUG(" -> davix_read");
    ssize_t ret =-1;
    DavixError* tmp_err=NULL;

    TRY_DAVIX{
        if( davix_check_rw_fd(fd, &tmp_err) ==0){
            ret = (ssize_t) fd->io_handler.read(fd->io_context, buf, (dav_size_t) count);
        }
    }CATCH_DAVIX(&tmp_err)

    DavixError::propagateError(err, tmp_err);
    DAVIX_DEBUG(" davix_read <-");
    return ret;
}


ssize_t DavPosix::pread(DAVIX_FD* fd, void* buf, size_t count, off_t offset, DavixError** err){
    DAVIX_DEBUG(" -> davix_pread");
    ssize_t ret =-1;
    DavixError* tmp_err=NULL;

    TRY_DAVIX{
        if( davix_check_rw_fd(fd, &tmp_err) ==0){
            ret = (ssize_t) fd->io_handler.pread(fd->io_context, buf, (dav_size_t)  count, (dav_off_t) offset);
        }
    }CATCH_DAVIX(&tmp_err)

    DavixError::propagateError(err, tmp_err);;
    DAVIX_DEBUG(" davix_pread <-");
    return ret;
}


ssize_t DavPosix::pwrite(DAVIX_FD* fd, const void* buf, size_t count, off_t offset, DavixError** err){
    DAVIX_DEBUG(" -> davix_pwrite");
    DAVIX_DEBUG(" davix_pwrite <-");
    return -1;
}

dav_ssize_t DavPosix::preadVec(DAVIX_FD* fd, const DavIOVecInput * input_vec,
                      DavIOVecOuput * output_vec,
                      dav_size_t count_vec, DavixError** err){
    DAVIX_DEBUG(" -> davix_pread_vec");
    ssize_t ret =-1;
    DavixError* tmp_err=NULL;

    TRY_DAVIX{
        if( davix_check_rw_fd(fd, &tmp_err) ==0){
            ret = fd->io_handler.preadVec(fd->io_context, input_vec, output_vec, count_vec);
        }
    }CATCH_DAVIX(&tmp_err)

    DavixError::propagateError(err, tmp_err);
    DAVIX_DEBUG(" davix_pread_vec <-");
    return ret;
}

ssize_t DavPosix::write(DAVIX_FD* fd, const void* buf, size_t count, Davix::DavixError** err){
    DAVIX_DEBUG(" -> davix_write");
    ssize_t ret =-1;
    DavixError* tmp_err=NULL;

    TRY_DAVIX{
        if( davix_check_rw_fd(fd, &tmp_err) ==0){
            ret = (ssize_t) fd->io_handler.write(fd->io_context, buf, (dav_size_t) count);
        }
    }CATCH_DAVIX(&tmp_err)

    DavixError::propagateError(err, tmp_err);
    DAVIX_DEBUG(" davix_write <-");
    return ret;
}



off_t DavPosix::lseek(DAVIX_FD* fd, off_t offset, int flags, Davix::DavixError** err){
    DAVIX_DEBUG(" -> davix_lseek");
    ssize_t ret =-1;
    DavixError* tmp_err=NULL;

    TRY_DAVIX{
        if( davix_check_rw_fd(fd, &tmp_err) ==0){
            ret = (off_t) fd->io_handler.lseek(fd->io_context, static_cast<dav_off_t>(offset), flags);
        }
    }CATCH_DAVIX(&tmp_err)

    DavixError::propagateError(err, tmp_err);
    DAVIX_DEBUG(" davix_lseek <-");
    return ret;
}


int DavPosix::close(DAVIX_FD* fd, Davix::DavixError** err){
    TRY_DAVIX{
        if(fd){
            fd->io_handler.resetIO(fd->io_context);
            delete fd;
        }
    }CATCH_DAVIX(err)
    return 0;
}


} // namespace Davix
