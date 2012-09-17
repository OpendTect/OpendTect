#ifndef uiicons_h
#define uiicons_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2009
 RCS:           $Id: uiicons.h,v 1.6 2010/11/16 11:26:58 cvsbert Exp $
________________________________________________________________________

-*/

#include "commondefs.h"

class ioPixmap;

namespace uiIcon
{

    mGlobal const char*		save();
    mGlobal const char*		saveAs();
    mGlobal const char*		openObject();
    mGlobal const char*		newObject();
    mGlobal const char*		removeObject();

    mGlobal const char*		None();
    				//!< Avoids pErrMsg

};


#endif
