#ifndef uiodhelpmenumgr_h
#define uiodhelpmenumgr_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Aug 2005
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "sets.h"

class uiODMenuMgr;
class uiMenu;


/*!\brief The OpendTect help menu manager */

mExpClass(uiODMain) uiODHelpMenuMgr
{
public:
    				uiODHelpMenuMgr(uiODMenuMgr*);
    				~uiODHelpMenuMgr();

    void			handle(int);

protected:

    uiODMenuMgr*		mnumgr_;
    uiMenu*			helpmnu_;
};

#endif

