#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		April 2021
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

    BufferString	hostname() const;

protected:

    uiGenInput*		hostnmfld_;
    uiGenInput*		hostnmoverrulefld_;
    uiGenInput*		hostaddrfld_;

    void		overrulecheckedCB(CallBacker*);
    void		hostnmoverruleCB(CallBacker*);
    void		lookupaddrCB(CallBacker*);
    bool		overruleOK() const;
};
