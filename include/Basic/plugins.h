#ifndef plugins_h
#define plugins_h


#include "gendefs.h"

#ifdef __cpp__
extern "C" {
#endif


/*! Plugins are loaded from shared libs or DLLs. For these things to
 be in any way useful, an init function must be called. The name of that 
 function should predictable. It is constructed as follows:
 xxxInitPlugin
 where xxx is the name of the plugin file, where:
 libxxx.so -> xxx 
 xxx.dll -> xxx 
 etc.

 The signature is:

 const char* xxxInitPlugin(int*,char**);

 An optional extra, if you want the plugin to be loaded automatically at
 startup is:

 int xxxGetPluginType(void);

 if not defined, PI_AUTO_INIT_NONE is assumed.
 */

#define PI_AUTO_INIT_NONE	0
#define PI_AUTO_INIT_EARLY	1
#define PI_AUTO_INIT_LATE	2


/*! To be called from program (once for EARLY, once for LATE) */
void LoadAutoPlugins(int* pargc,char** argv,int inittype);
/*! To be called from program if needed */
bool LoadPlugin(const char* libnm,int* pargc,char** argv);


#ifdef __cpp__
}
#endif

#endif
