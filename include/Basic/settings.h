#ifndef Settings_H
#define Settings_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		4-11-1995
 RCS:		$Id: settings.h,v 1.3 2000-10-17 16:01:37 bert Exp $
________________________________________________________________________


-*/

/*! \brief Settings hold the user settings. It is an IOPar.

The .dGBSettings or .dGBSettings.$dGB_USER in the user's HOME will be read.
*/


#include <iopar.h>


class Settings : public IOPar
{
public:

			Settings(const char* filename=0);
			~Settings();

    bool		reRead();
    bool		write(bool read_before=true) const;
			//!< read_before should be true: this is the protection
			//!< against another update being screwed by this one

    inline static Settings& common()
			{
			    if ( !common_ ) common_ = new Settings;
			    return *common_;
			}

protected:

    FileNameString	fname;

    static Settings*	common_;

};


//! Below macros for easy get and set to Settings::common()

#define mSettUse(fn,basekey,key,value) \
	Settings::common().fn( IOPar::compKey(basekey,key), value )
#define mSettGet(basekey,key) \
	Settings::common()[ IOPar::compKey(basekey,key) ]



#endif
