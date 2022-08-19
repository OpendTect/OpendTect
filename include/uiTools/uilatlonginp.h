#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"

class LatLong;
class uiGenInput;
class uiLatLongDMSInp;
class uiLineEdit;
class uiRadioButton;

namespace Coords { class CoordSystem; }


mExpClass(uiTools) uiLatLongInp : public uiGroup
{ mODTextTranslationClass(uiLatLongInp)
public:

			uiLatLongInp(uiParent*);
			~uiLatLongInp();

    void		get(LatLong&) const;
    void		set(const LatLong&);
    void		setReadOnly(bool);

    Notifier<uiLatLongInp> valueChanged;

protected:

    uiRadioButton*	isdecbut_;
    uiLineEdit*		latdecfld_;
    uiLineEdit*		lngdecfld_;
    uiLatLongDMSInp*	latdmsfld_;
    uiLatLongDMSInp*	lngdmsfld_;

    void		get(LatLong&,bool) const;
    void		set(const LatLong&,int);

    void		typSel(CallBacker*);
    void		llchgCB(CallBacker*);
};
