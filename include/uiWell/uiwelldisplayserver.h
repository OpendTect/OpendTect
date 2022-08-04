#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		Nov 2021
________________________________________________________________________

-*/
#include "uiwellmod.h"
#include "uidialog.h"
#include "uiwelllogtools.h"


mExpClass(uiWell) uiWellDisplayServer
{ mODTextTranslationClass(uiWellDisplayServer);
public:
			uiWellDisplayServer();
    virtual		~uiWellDisplayServer();

    virtual uiDialog*	createMultiWellDisplay(uiParent*, const DBKeySet&,
					       const BufferStringSet&) = 0;
    virtual uiWellLogToolWinGrp*	createWellLogToolGrp(uiParent*,
			    const ObjectSet<uiWellLogToolWin::LogData>&) = 0;
    virtual bool	isBasic() const { return false; }
};


mExpClass(uiWell) uiODWellDisplayServer : public uiWellDisplayServer
{ mODTextTranslationClass(uiODWellDisplayServer);
public:
			uiODWellDisplayServer();
			~uiODWellDisplayServer();

    uiDialog*		createMultiWellDisplay(uiParent*, const DBKeySet&,
					    const BufferStringSet&) override;
    uiWellLogToolWinGrp*	createWellLogToolGrp(uiParent*,
			const ObjectSet<uiWellLogToolWin::LogData>&) override;
    bool		isBasic() const override	{ return true; }
};

extern "C++" {
    mGlobal(uiWell) uiWellDisplayServer& GetWellDisplayServer(bool set=false,
					    uiWellDisplayServer* wds = nullptr);
}
