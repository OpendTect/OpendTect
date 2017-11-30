#ifndef uiodlangmenumgr_h
#define uiodlangmenumgr_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		Nov 2017
 RCS:		$Id$
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

#endif
