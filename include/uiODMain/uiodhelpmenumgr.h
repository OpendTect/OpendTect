#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			uiODHelpMenuMgr(uiODMenuMgr*);
			~uiODHelpMenuMgr();

    void		handle(int);
    uiMenu*		getDocMenu();

protected:

    uiString		getAboutString();
    void		showLegalInfo();
    void		showShortKeys();

    uiODMenuMgr*	mnumgr_;
    uiMenu*		helpmnu_;
    uiMenu*		docmnu_;
};
