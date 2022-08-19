#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidlggroup.h"

namespace visSurvey { class PlaneDataDisplay; }

class uiButtonStateEdit;

mExpClass(uiVis) uiVisPlaneDataDisplayDragProp : public uiDlgGroup
{ mODTextTranslationClass(uiVisPlaneDataDisplayDragProp);
public:
    			uiVisPlaneDataDisplayDragProp(uiParent*,
			    visSurvey::PlaneDataDisplay* );
    bool		acceptOK();
    bool		revertChanges();

protected:
    visSurvey::PlaneDataDisplay*	pdd_;
    uiButtonStateEdit*			scrollstate_;
    uiButtonStateEdit*			panstate_;

    const int				initialscrollstate_;
    const int				initialpanstate_;
};
