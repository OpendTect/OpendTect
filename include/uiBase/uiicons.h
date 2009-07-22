#ifndef uiicons_h
#define uiicons_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2009
 RCS:           $Id: uiicons.h,v 1.4 2009-07-22 16:01:21 cvsbert Exp $
________________________________________________________________________

-*/

/*! Commonly used icons. */
#include "commondefs.h"

class ioPixmap;

namespace Icons
{
    mGlobal const ioPixmap&	save();
    mGlobal const ioPixmap&	saveAs();
    mGlobal const ioPixmap&	openObject();
    mGlobal const ioPixmap&	newObject();
    mGlobal const ioPixmap&	removeObject();
};


#endif
