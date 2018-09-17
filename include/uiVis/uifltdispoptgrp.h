#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		November 2011
________________________________________________________________________


-*/

#include "uivismod.h"
#include "uidlggroup.h"

class uiCheckBox;
class uiGenInput;
class uiPushButton;
namespace visSurvey { class FaultDisplay; }

mExpClass(uiVis) uiFaultDisplayOptGrp : public uiDlgGroup
{ mODTextTranslationClass(uiFaultDisplayOptGrp);
public:
				uiFaultDisplayOptGrp(uiParent*,
						     visSurvey::FaultDisplay*);

    void			setFaultDisplay(visSurvey::FaultDisplay*);
    bool			acceptOK();

protected:

    bool			apply();
    void			applyCB(CallBacker*);
    void			algChg(CallBacker*);
    void			dispChg(CallBacker*);

    uiGenInput*			algfld_;
    uiPushButton*		applybut_;

    uiCheckBox*			sectionsfld_;
    uiCheckBox*			horizonsfld_;
    uiCheckBox*			planesfld_;
    uiCheckBox*			sticksfld_;
    uiCheckBox*			nodesfld_;

    visSurvey::FaultDisplay*	fltdisp_;
};
