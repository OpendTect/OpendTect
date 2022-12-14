#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "uistrings.h"

class uiGenInput;


mExpClass(uiTools) uiLocalHostGrp : public uiGroup
{ mODTextTranslationClass(uiLocalHostGrp);
public:

			uiLocalHostGrp(uiParent*, const uiString& txt=
				 tr("Computer/Host"),
				 bool withoverride=true);
			~uiLocalHostGrp();

    void		setHSzPol(uiObject::SzPolicy);

    BufferString	hostname() const;
    BufferString	address() const;

protected:

    uiGenInput*		hostnmfld_;
    uiGenInput*		hostnmoverrulefld_;
    uiGenInput*		hostaddrfld_;

    void		overrulecheckedCB(CallBacker*);
    void		hostnmoverruleCB(CallBacker*);
    void		lookupaddrCB(CallBacker*);
    bool		overruleOK() const;
};
