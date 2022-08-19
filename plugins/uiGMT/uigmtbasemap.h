#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidlggroup.h"

class uiCheckBox;
class uiGenInput;
class uiPushButton;
class uiTextEdit;

mClass(uiGMT) uiGMTBaseMapGrp : public uiDlgGroup
{ mODTextTranslationClass(uiGMTBaseMapGrp);
public:

    			uiGMTBaseMapGrp(uiParent*);

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		reset();

protected:

    uiGenInput*		titlefld_;
    uiGenInput*		xdimfld_;
    uiGenInput*		ydimfld_;
    uiGenInput*		scalefld_;
    uiGenInput*		xrgfld_;
    uiGenInput*		yrgfld_;
    uiPushButton*	resetbut_;
    uiGenInput*		lebelintvfld_;
    uiCheckBox*		gridlinesfld_;
    uiTextEdit*		remarkfld_;

    float		aspectratio_;

    void		resetCB(CallBacker*);
    void		xyrgChg(CallBacker*);
    void		dimChg(CallBacker*);
    void		scaleChg(CallBacker*);
    void		updateFlds(bool);
};
