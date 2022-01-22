#pragma once  
#define MP_VERSION_MAJOR 2 
#define MP_VERSION_MINOR 0 
#define MP_BUILD 641
#define MP_BRANCH 0 
#define MAKE_TOKEN(token) #token 
#define MAKE_VERSION_STRING(major, minor, build, branch) MAKE_TOKEN(major) "." MAKE_TOKEN(minor) "." MAKE_TOKEN(build) "." MAKE_TOKEN(branch)
#define MP_VERSION_STR MAKE_VERSION_STRING(MP_VERSION_MAJOR, MP_VERSION_MINOR, MP_BUILD, MP_BRANCH)
