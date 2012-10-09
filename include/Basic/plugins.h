#ifndef plugins_h
#define plugins_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Aug 2003
 Contents:	Plugins

 RCS:		$Id$

 For making your own plugins, no need to include this file. Use odplugin.h.
________________________________________________________________________

-*/

#include "sharedlibs.h"
#include "bufstring.h"
#include "objectset.h"


class FileMultiString;

#ifdef __cpp__
extern "C" {
#endif

#include "pluginbase.h"

/* C Access. C++ should use PluginManager! */

/*! To be called from program (once for EARLY, once for LATE) */
void LoadAutoPlugins(int argc,char** argv,int inittype);
/*! To be called from program if needed */
int LoadPlugin(const char* libnm);

#ifdef __cpp__
}

/*!\brief Plugin manager - loads plugins: shared libs or DLLs.

Note: there are macros making most of the below stuff not interesting.
See header file odplugin.h.
 
 For shared libs to be in any way useful, an init function
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

 Optional extras:

 1) If you want the plugin to be loaded automatically at
 startup define:

 extern "C" int GetxxxPluginType(void);

 if not defined, PI_AUTO_INIT_NONE is assumed, which means it will not be loaded
 if not explicitly done so.

 Loading from startup is done from $HOME/.od/plugins/$PLFSUBDIR/libs or
 $dGB_APPL/plugins/$PLFSUBDIR/libs. The plguniins in these directories will
 be loaded only if they are mentioned in a .alo file in the parent directory,
 $HOME/.od/plugins/$PLFSUBDIR or $dGB_APPL/plugins/$PLFSUBDIR.
 The alo files are handled in alphabetical order.

 2) It may be a good idea to define a function:

 extern "C" PluginInfo* GetxxxPluginInfo(void);

 Make sure it returns an object of type PluginManager::Info*. Make sure it
 points to an existing object (static or made with new/malloc);

3) The user of PIM() can decide not to load all of the .alo load libs. After
   setArgs() (which is absolutely mandatory), the getData() list is filled.
   You can change the source_ to None before calling loadAuto().

 */

mClass PluginManager
{
public:

    mGlobal friend PluginManager& PIM();
    
    static PluginManager&	getInstance();

    void			setArgs(int argc,char** argv);
    					//!< Mandatory
    void			loadAuto(bool late);
    					//!< see class comments
    bool			load(const char* libnm);
    					//!< Explicit load of a plugin

    struct Data
    {
	enum AutoSource		{ None, UserDir, AppDir, Both };
	static bool		isUserDir( AutoSource src )
	    			{ return src != AppDir && src != None; }

				Data( const char* nm )
				    : name_(nm)
				    , info_(0)
				    , autosource_(None)
				    , autotype_(PI_AUTO_INIT_NONE)
				    , isloaded_( false )
				    , sla_(0)	{}
				~Data()		{ delete sla_; }

	BufferString		name_;
	const PluginInfo*	info_;
	AutoSource		autosource_;
	int			autotype_;
	SharedLibAccess*	sla_;
	bool			isloaded_;
    };

    ObjectSet<Data>&	getData()		{ return data_; }
    Data*		findData( const char* nm ) { return fndData( nm ); }
    const Data*		findData( const char* nm ) const {return fndData( nm );}
    const Data*		findDataWithDispName(const char*) const;

    bool		isPresent(const char*) const;
    const char*		userName(const char*) const;
    			//!< returns without path, 'lib' and extension
    static const char*	moduleName(const char*);
			//!< returns without path, 'lib' and extension

    const char*		getFileName(const Data&) const;

    const char*		getAutoDir( bool usr ) const
			{ return usr ? userlibdir_ : applibdir_; }

    static const char*	sKeyDontLoad() { return "dTect.Dont load plugins"; }
    void		getNotLoadedByUser(FileMultiString&) const;

private:

    				PluginManager();
    
    int				argc_;
    char**			argv_;
    ObjectSet<Data>		data_;

    BufferString		userdir_;
    BufferString		appdir_;
    BufferString		userlibdir_;
    BufferString		applibdir_;

    Data*			fndData(const char*) const;
    void			getDefDirs();
    void			getALOEntries(const char*,bool);
    void			openALOEntries();
    void			mkALOList();

};

mGlobal PluginManager& PIM();


#endif /* End of C++ only section */


#endif
