#ifndef plugins_h
#define plugins_h
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Aug 2003
 Contents:	Plugins
 RCS:		$Id: plugins.h,v 1.11 2003-11-17 14:43:15 bert Exp $
________________________________________________________________________

-*/


#include "bufstringset.h"

#ifdef __cpp__
extern "C" {
#endif

#define PI_AUTO_INIT_NONE	0
#define PI_AUTO_INIT_EARLY	1
#define PI_AUTO_INIT_LATE	2

/*!\brief Information about plugin for outside world */

typedef struct {

    const char*	dispname;
    const char*	creator;
    const char*	version;
    const char*	text;

} PluginInfo;

/* C Access. C++ should use PluginManager! */

/*! To be called from program (once for EARLY, once for LATE) */
void LoadAutoPlugins(int argc,char** argv,int inittype);
/*! To be called from program if needed */
bool LoadPlugin(const char* libnm,int argc,char** argv);


#ifdef __cpp__
}

/*!\brief Plugin manager - loads plugins: shared libs or DLLs.
 
 For shared libs things to be in any way useful, an init function
 must be called. The name of that function should predictable.
 It is constructed as follows:
 InitxxxPlugin
 where xxx is the name of the plugin file, where:
 libxxx.so -> xxx 
 xxx.dll -> xxx 
 etc.

 The signature is:

 extern "C" {
 const char* InitxxxPlugin(int,char**);
 }

 Optional extras.

 1) If you want the plugin to be loaded automatically at
 startup define:

 extern "C" int GetxxxPluginType(void);

 if not defined, PI_AUTO_INIT_NONE is assumed, which means it will not be loaded
 if not explicitly done so.

 Loading from startup is done from $HOME/.odplugins/$HDIR or $dGB_APPL/...
 Plugins can always be put in these directory (for all) or in a subdirectory
 with the name of the program (e.g. dtectmain).
 From OpendTect V1.0.3 we also support the '.alo' files with the names of
 the plugins (e.g. DipSteer), and the plugins themeselves in the 'libs'
 subdirectory.
 The non-alo plugins will be loaded in alphabetical order, for .alo files the
 order specified in the file. The alo files themselves are handled in
 alphabetical order.

 2) It may be a good idea to define a function:

 extern "C" PluginInfo* GetxxxPluginInfo(void);

 Make sure it returns an object of type PluginManager::Info*. Make sure it
 points to an existing object (static or made with new/malloc);

 */

class PluginManager
{
public:

    void			setArgs( int argc, char** argv )
				{ argc_ = argc, argv_ = argv; }
    bool			load( const char* libnm )
				{ return LoadPlugin(libnm,argc_,argv_); }
    void			loadAuto( bool late=true )
				{ LoadAutoPlugins( argc_, argv_,
				  late?PI_AUTO_INIT_LATE:PI_AUTO_INIT_EARLY); }

    bool			isLoaded(const char*); //!< file or username
    const BufferStringSet&	loadedFileNames() const
				{ return loaded_; }
    const char*			userName(const char*) const;
    				//!< returns without 'lib', extension and path
    const PluginInfo&		getInfo(const char*) const;

    const char*			defDir(bool instdir=true) const;
    				//! base directoy where plugins are located

private:

    				PluginManager();
    friend PluginManager&	PIM();

    int				argc_;
    char**			argv_;
    static PluginManager*	theinst_;
    BufferStringSet		loaded_;
    ObjectSet<PluginInfo>	info_;

    void			getUsrNm(const char*,BufferString&) const;
    const char*			getFullName(const char*) const;

public:

    void			addLoaded( const char* s, PluginInfo* p )
				{ loaded_ += new BufferString(s); info_ += p; }
    				//!< Don't use.
};

inline PluginManager& PIM()
{
    if ( !PluginManager::theinst_ )
	PluginManager::theinst_ = new PluginManager;
    return *PluginManager::theinst_;
}

#endif

#endif
