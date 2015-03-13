#ifndef uibasemapiomgr_h
#define uibasemapiomgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		January 2015
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemapmod.h"
#include "uibasemapitem.h"

class uiParent;

mExpClass(uiBasemap) uiBasemapIOMgr
{mODTextTranslationClass(uiBasemapIOMgr)
public:

		uiBasemapIOMgr(uiParent*);
		~uiBasemapIOMgr();

    bool	save(bool saveas);
    bool	read(bool haschanged);

private:
    uiParent*		parent_;
    MultiID		curbasemapid_;
};

#endif
