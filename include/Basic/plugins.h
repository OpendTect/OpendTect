#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "sharedlibs.h"
#include "bufstringset.h"
#include "notify.h"


extern "C" {

#include "pluginbase.h"

/* C Access. C++ should use PluginManager! */

/*! To be called from program (once for EARLY, once for LATE) */
mGlobal(Basic) void LoadAutoPlugins(int inittype);
/*! To be called from program if needed */
mGlobal(Basic) int LoadPlugin(const char* libnm);

}

/*!
\brief Plugin manager - loads plugins: shared libs or DLLs.

  Note: there are macros making most of the below stuff not interesting.
  See header file odplugin.h.

  For making your own plugins, no need to include this file. Use odplugin.h.

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

  if not defined, PI_AUTO_INIT_NONE is assumed, which means it will not be
  loaded if not explicitly done so.

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
  construction, the getData() list is filled. You can change the source_ to
  None before calling loadAuto().
*/

mExpClass(Basic) PluginManager : public CallBacker
{
public:

    mGlobal(Basic) friend PluginManager& PIM();

				~PluginManager();

    static PluginManager&	getInstance();

    void			loadAuto(bool late,bool withfilter=false);
				//!< see class comments
    bool			load(const char* libnm);
					//!< Explicit load of a plugin
    bool			unload(const char* libnm);
					//!< Explicit unload of a plugin
    void			unLoadAll();

    mExpClass(Basic) Data
    {
    public:
	enum AutoSource		{ None, UserDir, AppDir, Both };

				Data(const char* nm);
				~Data();
				mOD_DisableCopy(Data)

	BufferString		getFileName() const;
				//!< Always without path
	BufferString		getBaseName() const;
				/*!< Without prefix and suffix, for example
					returns 'Basic' from 'libBasic.so' */

	BufferString		name_;
	const PluginInfo*	info_		= nullptr;
	AutoSource		autosource_	= None;
	int			autotype_	= PI_AUTO_INIT_NONE;
	SharedLibAccess*	sla_		= nullptr;
	bool			isloaded_	= false;

	static bool		isUserDir(AutoSource);
    };

    bool			load(const char* libnm,Data::AutoSource,
				     int autotype);

    ObjectSet<Data>&	getData()		{ return data_; }
    Data*		findData( const char* nm ) { return fndData( nm ); }
    const Data*		findData( const char* nm ) const {return fndData( nm );}
    const Data*		findDataWithDispName(const char*) const;

    bool		isPresent(const char*) const;
    const char*		userName(const char*) const;
			/*!<returns name in plugin-info, or base name
			    if plugin info is not available. */

    BufferString	getFileName(const Data&) const;
			//!< return filename with full path

    const char*		getAutoDir( bool usr ) const
			{ return usr ? userlibdir_ : applibdir_; }

    static const char*	sKeyDontLoad() { return "dTect.Dont load plugins"; }
    void		getNotLoadedByUser(BufferStringSet&) const;

private:

				PluginManager();

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

mGlobal(Basic) PluginManager& PIM();

mGlobal(Basic) void initPluginClasses(const char* datadir,const char* funcnm);
