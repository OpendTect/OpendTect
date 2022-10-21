#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uifuncdispbase.h"
#include "uifuncdrawerbase.h"

class uiParent;

mExpClass(uiTools) uiFunctionDisplayServer
{ mODTextTranslationClass(uiFunctionDisplayServer);
public:
			uiFunctionDisplayServer();
    virtual		~uiFunctionDisplayServer();

    virtual uiFuncDispBase*	createFunctionDisplay(uiParent*,
					const uiFuncDispBase::Setup&) = 0;
    virtual uiFuncDrawerBase*	createFunctionDrawer(uiParent*,
					const uiFuncDrawerBase::Setup&) = 0;

    virtual bool	isBasic() const { return false; }
};


mExpClass(uiTools) uiODFunctionDisplayServer : public uiFunctionDisplayServer
{ mODTextTranslationClass(uiODFunctionDisplayServer);
public:
			uiODFunctionDisplayServer();
			~uiODFunctionDisplayServer();

    uiFuncDispBase*	createFunctionDisplay(uiParent*,
				    const uiFuncDispBase::Setup&) override;
    uiFuncDrawerBase*	createFunctionDrawer(uiParent*,
				    const uiFuncDrawerBase::Setup&) override;

    bool		isBasic() const override	{ return true; }
};

extern "C++" {
    mGlobal(uiTools) uiFunctionDisplayServer& GetFunctionDisplayServer(
					bool set=false,
					uiFunctionDisplayServer* fds = nullptr);
}
