#include <davix.hpp>
#include <string>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <gtest/gtest.h>

using namespace Davix;

static const char* list_urls[] = {
    "http://localhost:80/",
    "sfdfsdfdsfds://localhost:rrrrrrr/path",
    "http://monurlrandom/",
    "www.google.com"
};

static const size_t list_port[]={
    80,
    0,
    0,
    80
};

static const char* list_host[] ={
    "localhost",
    "localhost",
    "monurlrandom",
    "nothing"
};

static const char* list_path[] ={
    "/",
    "/path",
    "/",
    "nothing",
};

static const char* list_proto[] ={
    "http",
    "http",
    "http",
    "nothing"
};

static bool failure[]={
    false,
    true,
    false,
    true
};

const size_t len_list= (sizeof(list_proto))/(sizeof(const char*));


TEST(testNeon, testParsing){
    davix_set_log_level(DAVIX_LOG_ALL);
    for(size_t i=0; i < len_list; ++i){
        Uri uri(list_urls[i]);
        if(failure[i] == false){
            ASSERT_EQ(StatusCode::OK, uri.getStatus());
            ASSERT_STREQ(list_proto[i], uri.getProtocol().c_str() );
            ASSERT_STREQ(list_host[i], uri.getHost().c_str());
            ASSERT_STREQ(list_path[i], uri.getPath().c_str());
            ASSERT_EQ(list_port[i], uri.getPort());
        }else{
            ASSERT_EQ(StatusCode::UriParsingError, uri.getStatus());
        }
    }
}


TEST(testUri, testCopyEQ){
    Uri uri;
    std::ostringstream ss;
    ASSERT_EQ(uri.getStatus(), StatusCode::UriParsingError);
    uri = Uri("http://wikipedia.org:80/bonus?salut");
    ASSERT_EQ(uri.getStatus(), StatusCode::OK);

    ss << uri;
    ASSERT_STREQ(ss.str().c_str(),uri.getString().c_str());

    std::string pathAndString("http://wikipedia.org:80/bonus?salut=bob");
    uri = pathAndString;
    std::string::reverse_iterator rit = std::find(pathAndString.rbegin(), pathAndString.rend(), '/');
    std::string::iterator it = pathAndString.begin() + (pathAndString.rend() - rit);
    ASSERT_STREQ(uri.getPathAndQuery().c_str(), std::string(it-1, pathAndString.end()).c_str() );

    Uri uri2(uri);
    ASSERT_TRUE(uri == uri2);
}



TEST(testUri, testEscape){

    std::string s("http://wikipedia.org:80/bonus a toi \\ é voilù ?salut");

    std::string r = Davix::Uri::unescapeString(Davix::Uri::escapeString(s));
    ASSERT_STREQ(s.c_str(), r.c_str());
}
