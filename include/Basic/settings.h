#ifndef Settings_H
#define Settings_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		4-11-1995
 RCS:		$Id: settings.h,v 1.2 2000-08-08 14:16:20 bert Exp $
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
    bool		write() const;

    inline static Settings& common()
			{
			    if ( !common_ ) common_ = new Settings;
			    return *common_;
			}

protected:

    FileNameString	fname;

    static Settings*	common_;

};


#endif
