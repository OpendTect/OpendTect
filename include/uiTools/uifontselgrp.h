#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
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
