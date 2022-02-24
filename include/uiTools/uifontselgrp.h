#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		December 2021
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "draw.h"
#include "fontdata.h"

class uiColorInput;
class uiToolButton;


mExpClass(uiTools) uiFontSelGrp : public uiGroup
{ mODTextTranslationClass(uiFontSelGrp);
public:
			uiFontSelGrp(uiParent*,const uiString& lbl,
				     bool withcolor=false,
				     OD::Color col=OD::Color(100,100,100));
			~uiFontSelGrp();

    void		setFont(const FontData&);
    FontData		getFont() const;
    void		setColor(const OD::Color&);
    OD::Color		getColor() const;

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    Notifier<uiFontSelGrp>	changed;

protected:

    uiToolButton*	fontbut_;
    uiColorInput*	colorfld_ = nullptr;
    FontData		fontdata_;

    void		fontChgCB(CallBacker*);
    void		colChgCB(CallBacker*);
};
