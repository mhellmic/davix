
/**
 *
@mainpage Davix Documentation
@author Devresse Adrien ( adrien.devresse@cern.ch )

From CERN IT-GT-DMS ( lcgutil-support@cern.ch )
mailing list : davix-devel@cern.ch


<h2> DAVIX </h2>

Davix is a lightweight toolkit for remote file / object interactions
      with HTTP based protocols.

Davix offers a multi-layer API for the end user :

- High level POSIX file API, for convenience
- High level Object API, REST like
- Low level Request API, compose your own HTTP query like with curl. </br>

Davix supports Http, Webdav ( and S3 ) </br>
Davix integrate a Webdav / XML parser. </br>

<h2> DAVIX API :</h2>

C++ API : \ref davix.hpp

<h2> Davix is yet an other libcurl ? </h2>

No,

- libcurl defines itself as a "client side URL transfer". <br/>
 it provides "protocol level" API, you compose your http queries mannually.

- Davix offers a "file level" API. <br/>
 With Davix, you access to your data and do not have to care about the underlying queries
 Davix tends to be of one level higher and provides a complete API for
 remote I/O and remote file management.

<h2> Examples : </h2>

<h3> POSIX API </h3>

 @code{.cpp}

            Davix::DavPosix p;
            p.stat("https://mywebdav-server.org/mydir/", &stat, &tmp_err);
            //
            // read ops
            fd= p.open(NULL, "https://mywebdav-server.org/myfile.jpg", O_RDONLY, &tmp_err);
            p.read(fd, buffer, size, &tmp_err);
            p.pread(fd, buffer, size2, offset, &tmp_err);
            //
            // directory creation
            p.mkdir(NULL, "https://mywebdav-server.org/mnewdir");
 @endcode

<h3> LOW LEVEL REQUEST API </h3>

 @code{.cpp}

            Davix::HttpRequest req("https://restapi-server.org/rest")
            req.addHeaderField(...)
            req.setRequestMethod("PUT")
            //
            // execute your request
            req.executeRequest(...);
 @endcode

<h2> What does Davix support ? </h2>

Davix supports :
    - all common posix operation ( stat/opendir/readdir/open/read/write/close/mkdir )
    - Webdav and XML parsing
    - remote random I/O
    - client side credential in PEM and P12 format
    - proxy certificate and voms extension
    - transparent redirection
    - third party copy with Webdav
    - keep alive and session re-use.
    - Basic authentication scheme

Davix is going to support soon :
    - transparent meta-link support with failover
    - vector operations
    - kerberos auth
    - transparent caching


<h2> How to compile : </h2>

- Davix Dependencies :
   -  openssl
   -  libxml-2.0
   -  Doxygen ( optional, for doc generation )

- Davix Portability :
   - Should run on Windows and any POSIX compatible Operating system
   - Any contribution to support a new plateform is welcome
   - Tested on Debian, Ubuntu, Fedora 18/19, Scientific Linux 5/6
   - Tested on Windows under MinGW
   - Tested with clang and GCC.

- Compile :
    - " 1.svn export http://svn.cern.ch/guest/lcgdm/davix/trunk davix "
    - " 2.cd davix "
    - " 3.mkdir build; cd build"
    - " 4. cmake ../"
    - " 5. make "

- Generate doc :
    - * run cmake
    - make doc

- Compile and run unit tests :
    - cmake -DUNIT_TESTS=TRUE ../
    - make
    - make test

- Compile & execute func tests :
     * warning : functionals test needs davserver and a valid credential
    - " 4. cmake  -DFUNCTIONAL_TESTS=TRUE ../ "
    - " 5. . ../test/setup_test_env.sh
    - " 5.  make; make test"

- make RPMS :
    - ./packaging/bin/packager_rpm.sh ./packaging/rpm/specs/ ./
    - mock -r [mycfg] RPMS/davix-[...].src.rpm


<h2> Play with davix command line tool : </h2>
    davix has a set of command line tools for testing purpose and demonstration

        -> davix-ls: file listing
        -> davix-get: download operations
        -> davix: low level query composition

<h2> I wish to see XYZ feature in Davix : </h2>

    Any suggestion is welcome.

    please contact us on davix-devel@cern.ch ( CERN e-group & mailing list ) or personnaly on adrien.devresse@cern.ch

    Davix is an open source and free project and will stay like this.
    Any contribution is welcome

<h2> Davix Website : </h2>

        https://svnweb.cern.ch/trac/lcgutil/wiki

*/
