#pragma once  
#define MAKE_VERSION_STRING(major, minor, branch, build) #major "." #minor "." #build "." #branch
#define MP_VERSION_MAJOR 1 
#define MP_VERSION_MINOR 2 
#define MP_BUILD 597 
#define MP_BRANCH 0 
#define MP_VERSION_STR MAKE_VERSION_STRING(MP_VERSION_MAJOR, MP_VERSION_MINOR, MP_BUILD, MP_BRANCH)
