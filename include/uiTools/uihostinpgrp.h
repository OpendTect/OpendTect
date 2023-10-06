#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "hostdata.h"
#include "uigroup.h"
#include "uistrings.h"


class uiGenInput;


mExpClass(uiTools) uiHostInpGrp : public uiGroup
{ mODTextTranslationClass(uiHostInpGrp);
public:
    enum		LookupMode { StaticIP, NameDNS };
			mDeclareEnumUtils(LookupMode)

			uiHostInpGrp(uiParent*,
				     const uiString& txt=tr("Computer/Host"));
			~uiHostInpGrp();

    HostData		hostData() const;
    void		setHostData(const HostData&);

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    uiGenInput*		modefld_;
    uiGenInput*		hostnmfld_;
    uiGenInput*		hostaddrfld_;
    uiGenInput*		lookupfld_;
    HostData		hostdata_;

    void		modeChgCB(CallBacker*);
    void		lookupCB(CallBacker*);
    bool		isStaticIP() const;
};
