#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		Jan 2022
________________________________________________________________________

-*/
#include "uitoolsmod.h"
#include "uifuncdispbase.h"
#include "uidialog.h"

mExpClass(uiTools) uiFunctionDisplayServer
{ mODTextTranslationClass(uiFunctionDisplayServer);
public:
			uiFunctionDisplayServer();
    virtual		~uiFunctionDisplayServer();

    virtual uiFuncDispBase*	createFunctionDisplay(uiParent*,
					const uiFuncDispBase::Setup&) = 0;
    virtual bool	isBasic() const { return false; }
};


mExpClass(uiTools) uiODFunctionDisplayServer : public uiFunctionDisplayServer
{ mODTextTranslationClass(uiODFunctionDisplayServer);
public:
			uiODFunctionDisplayServer();
			~uiODFunctionDisplayServer();

    uiFuncDispBase*	createFunctionDisplay(uiParent*,
				    const uiFuncDispBase::Setup&) override;
    bool		isBasic() const override	{ return true; }
};

extern "C" {
    mGlobal(uiTools) uiFunctionDisplayServer& GetFunctionDisplayServer(
					bool set=false,
					uiFunctionDisplayServer* fds = nullptr);
}
