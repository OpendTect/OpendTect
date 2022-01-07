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

class uiGenInput;
class uiPushButton;


mExpClass(uiTools) uiFontSelGrp : public uiGroup
{ mODTextTranslationClass(uiFontSelGrp);
public:
			uiFontSelGrp(uiParent*,const uiString& lbl,
				     bool withPosition=false);
			~uiFontSelGrp();

    void		setFont(const FontData&);
    FontData		getFont() const;
    void		setAlignment(const Alignment&);
    Alignment		getAlignment() const;
    Coord		getTextOffset() const;
    void		setTextOffset(Coord);

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    Notifier<uiFontSelGrp>	changed;

protected:

    uiPushButton*	fontbut_;
    uiGenInput*		halignfld_ = nullptr;
    uiGenInput*		valignfld_ = nullptr;
    uiGenInput*		offsetfld_ = nullptr ;
    bool		withpos_;
    FontData		fontdata_;

    void		fontChgCB(CallBacker*);
    void		propertyChgCB(CallBacker*);
};
