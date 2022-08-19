#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "callback.h"
#include "uistring.h"

class uiMenu;
class uiODMenuMgr;

/*!\brief The OpendTect language menu manager */

mExpClass(uiODMain) uiODLangMenuMgr : public CallBacker
{ mODTextTranslationClass(uiODLangMenuMgr)
public:
			uiODLangMenuMgr(uiODMenuMgr*);
			~uiODLangMenuMgr();

protected:

    void		initLanguageMenu();
    void		updateLanguageMenu();

    void		languageChangeCB(CallBacker*);
    void		languageSelectedCB(CallBacker*);

    uiODMenuMgr*	mnumgr_;
    uiMenu*		langmnu_;
};
