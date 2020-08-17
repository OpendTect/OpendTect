#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		November 2011
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uivismod.h"
#include "uidlggroup.h"


class uiGenInput;
class uiPushButton;
namespace visSurvey { class FaultDisplay; }

mExpClass(uiVis) uiFaultDisplayOptGrp : public uiDlgGroup
{ mODTextTranslationClass(uiFaultDisplayOptGrp);
public: 
    		 		uiFaultDisplayOptGrp(uiParent*,
						     visSurvey::FaultDisplay*);
    bool			acceptOK();

protected:

    bool			applyCB(CallBacker*);
    void			algChg(CallBacker*);

    uiGenInput*			algfld_;
    uiPushButton*		applybut_;
    visSurvey::FaultDisplay*	fltdisp_;
};


