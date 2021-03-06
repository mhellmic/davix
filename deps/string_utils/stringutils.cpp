#include <config.h>
#include "stringutils.hpp"
#include <cstring>
#include <algorithm>

namespace StrUtil{

std::vector<std::string> tokenSplit(const std::string & str, const std::string & delimiter){
    std::vector<std::string> res;
    std::string::const_iterator it_prev, it_cur;
    for(it_prev = it_cur = str.begin(); it_cur < str.end(); ((it_prev != str.end())?(it_prev++):(it_prev))){
        it_cur = std::find_first_of(it_prev, str.end(), delimiter.begin(), delimiter.end());
        if(it_prev != it_cur)
            res.push_back(std::string(it_prev,it_cur));
        it_prev = it_cur;
    }
    return res;
}


int compare_ncase(const std::string & str1, const std::string & str2){
    return strcasecmp(str1.c_str(), str2.c_str());
}


int compare_ncase(const std::string & str1, off_t offset, size_t size, const char* cstr2){
     return strncasecmp(str1.c_str(), cstr2+ offset, size);
}


}
