#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "iopar.h"

class SettingsManager;
class uiRetVal;

/*!
\brief Settings class holds the user settings. It is an IOPar.

  The common() settings are basic, global user settings.
  For a specific subject or from a plugin, just call fetch( yourkey ),
  and a new Settings instance will be made if necessary. You can provide
  defaults in a file yourkeySettings which must be located in OD's data
  subdirectory.

  The data is stored in ~/.od/settings (common) and ~/.od/settings_yourkey
  for other keys. If $DTECT_USER is set, '.$DTECT_USER' is appended to the
  filename.
*/

mExpClass(Basic) Settings : public IOPar
{
public:

    inline static Settings&	common() { return fetch(); }

    static Settings&		fetch(const char* settings_name=0);

    bool			write(bool read_before=true) const;
				//!< read_before should be true: this is the
				//!< protection against another update being
				//!< screwed by this one
    bool			reRead()	{ return doRead(false); }
				//!< Needed in case you know that the file has
				//!< been changed by user or external routine.
				//!< It's not likely that you'll need this.

    static Settings*		fetchExternal(const char* settings_name,
					      const char* dtect_user,
					      const char* usr_settings_dir);
				//!< for sysadm purposes

    static bool			settsFileExist(const char* settings_name);

protected:

				Settings( const char* fnm )
				    : fname_(fnm)		{}
				~Settings()			{}

    BufferString		fname_;

    static Settings*		doFetch(const char*,const char*,const char*,
					bool);
    bool			doRead(bool);

private:

    static void			manageSettings(Settings*);

    friend class SettingsManager;
};


mExternC(Basic) const char* GetSettingsDataDir(void);
mGlobal(Basic) bool SetSettingsDataDir(const char*,uiRetVal&);

mExternC(Basic) const char* GetSettingsSurveyDir(void);


//!> macro for easy set to Settings::common()
#define mSettUse(fn,basekey,key,value) \
	Settings::common().fn( IOPar::compKey(basekey,key), value )
//!> macro for easy get from Settings::common()
#define mSettGet(basekey,key) \
	Settings::common()[ IOPar::compKey(basekey,key) ]
//!> macro for easy write of Settings::common()
#define mSettWrite() \
	Settings::common().write();
