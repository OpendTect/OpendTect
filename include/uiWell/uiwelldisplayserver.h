#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
