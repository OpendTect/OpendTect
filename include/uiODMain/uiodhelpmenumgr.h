#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2005
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "bufstring.h"
#include "uistring.h"

class uiODMenuMgr;
class uiMenu;


/*!\brief The OpendTect help menu manager */

mExpClass(uiODMain) uiODHelpMenuMgr
{ mODTextTranslationClass(uiODHelpMenuMgr)
public:
			uiODHelpMenuMgr(uiODMenuMgr&);
			~uiODHelpMenuMgr();

    void		handle(int);
    uiMenu*		getDocMenu();

protected:

    uiString		getAboutString();
    void		showLegalInfo();
    void		showShortKeys();

    uiODMenuMgr&	mnumgr_;
    uiMenu*		helpmnu_;
    uiMenu*		docmnu_;

};
