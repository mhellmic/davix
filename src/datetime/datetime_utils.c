#include <config.h>
#include "datetime_utils.h"



time_t parse_http_date(const char* http_date){
        // TODO : possible issue with timezone != GMT,

    static const char rfc1123[] = "%a, %d %b %Y %H:%M:%S GMT";
    struct tm tm;
    memset(&tm,0, sizeof(struct tm));

    const char * p = strptime(http_date, rfc1123, &tm);
    if ( p == NULL || *p != '\0'){
        return -1;
    }
    return mktime(&tm) - timezone;;

}


time_t parse_iso8601date(const char* iso_date){

    // TODO : refactor to a regex parser.. ISO8601 b******* is impossible to parse correctly with strptime
    struct tm tm_time;
    char* p, *end_p;
    memset(&tm_time,0, sizeof(struct tm ));
    if(  ( (p = strptime(iso_date, "%Y-%m-%dT%H:%M:%SZ", &tm_time) ) == NULL || *p != '\0')
         && ( (p = strptime(iso_date, "%Y-%m-%dT%H:%M:%S", &tm_time) ) == NULL || *p != '.'
              ||  *(iso_date + strlen(iso_date) -1) != 'Z' )){
        if( (p = strptime(iso_date, "%Y-%m-%dT%H:%M:%S", &tm_time) ) != NULL
                && (*p == '+' || *p == '-') ){
                struct tm tm_time_offset;
                memset(&tm_time_offset,0, sizeof(struct tm ));
                if( ( ( end_p= strptime(p+1,  "%H:%M", &tm_time_offset)) == NULL
                      || *end_p != '\0'
                     )
                    && ( (end_p =strptime(p+1,  "%H%M", &tm_time_offset)) == NULL
                         || *end_p != '\0'
                       )
                   )
                    return -1;
                if( *p =='+'){
                    tm_time.tm_hour +=  tm_time_offset.tm_hour;
                    tm_time.tm_min +=  tm_time_offset.tm_min;
                 } else{
                    tm_time.tm_hour -=  tm_time_offset.tm_hour;
                    tm_time.tm_min -=  tm_time_offset.tm_min;
                 }
        }else{
            return -1;
        }
    }


    return mktime(&tm_time) - timezone;

}

time_t parse_standard_date(const char* http_date){
    if(strchr(http_date, ',') != NULL){ // detect if rfc date or iso8601 date
        return parse_http_date(http_date);
    }else{
         return parse_iso8601date(http_date);
    }
}
