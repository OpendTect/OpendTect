#ifndef uiclipboard_h
#define uiclipboard_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Aug 2014
 RCS:		$Id$
________________________________________________________________________

Wrapper class around QClipboard

-*/

#include "uibasemod.h"
#include "commondefs.h"

mFDQtclass(QImage);
mFDQtclass(QPixmap);
class uiString;


/*!Wrapper class around the QClipboard */

mExpClass(uiBase) uiClipboard
{
public:
    static void		setText(const uiString&);
    static void		setText(const char*);
    static void		setImage(const QImage&);
};


#endif
