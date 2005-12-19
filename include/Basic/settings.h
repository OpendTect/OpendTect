#ifndef Settings_H
#define Settings_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		4-11-1995
 RCS:		$Id: settings.h,v 1.7 2005-12-19 11:40:46 cvsbert Exp $
________________________________________________________________________


-*/

/*!\brief Settings hold the user settings. It is an IOPar.

  The common() settings are for OpendTect itself. If a plugin needs some
  stored defaults, just call fetch( yourkey ), and a new Settings
  instance will be made. You can provide defaults in a file yourkeySettings
  which must be located in OD's data subdirectory.

  The data is stored in ~/.od/settings (common) and ~/.od/settings_yourkey
  for other keys. If $DTECT_USER is set, '.$DTECT_USER' is appended to the
  filename.

*/


#include <iopar.h>


class Settings : public IOPar
{
public:

    inline static Settings&	common() { return fetch(); }

    static Settings&		fetch(const char* settings_name=0);

    bool			write(bool read_before=true) const;
				//!< read_before should be true: this is the
    				//!< protection against another update being
    				//!< screwed by this one
    bool			reRead();
				//!< Needed in case you know that the file has
    				//!< been changed by user or external routine
    				//!< Not likely.


protected:

				Settings( const char* fnm )
				    : fname(fnm)		{}
				~Settings()			{}

    FileNameString		fname;
};


//!> macro for easy set to Settings::common()
#define mSettUse(fn,basekey,key,value) \
	Settings::common().fn( IOPar::compKey(basekey,key), value )
//!> macro for easy get from Settings::common()
#define mSettGet(basekey,key) \
	Settings::common()[ IOPar::compKey(basekey,key) ]

#endif
