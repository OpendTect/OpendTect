#ifndef Settings_H
#define Settings_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		4-11-1995
 RCS:		$Id: settings.h,v 1.6 2003-11-07 12:21:51 bert Exp $
________________________________________________________________________


-*/

/*!\brief Settings hold the user settings. It is an IOPar.

  The common() settings are for OpendTect itself. If a plugin needs some
  stored defaults, just call fetch() with your own key, and a new Settings
  instance will be made.

  The data is stored in ~/.od/settings (common) and ~/.od/settings.xxx
  for other keys.

*/


#include <iopar.h>


class Settings : public IOPar
{
public:

    inline static Settings&	common() { return fetch(0); }

    static Settings&		fetch(const char* settings_name);

    bool			write(bool read_before=true) const;
				//!< read_before should be true: this is the
    				//!< protection against another update being
    				//!< screwed by this one
    bool			reRead()	{ return doReRead(false); }
				//!< In case you know that the file has changed


protected:

				Settings( const char* fnm )
				    : fname(fnm)	{}
				~Settings()		{}

    FileNameString		fname;

    bool			doReRead(bool);

};


//!> macro for easy set to Settings::common()
#define mSettUse(fn,basekey,key,value) \
	Settings::common().fn( IOPar::compKey(basekey,key), value )
//!> macro for easy get from Settings::common()
#define mSettGet(basekey,key) \
	Settings::common()[ IOPar::compKey(basekey,key) ]

#endif
