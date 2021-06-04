#pragma once
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		9-3-1999
________________________________________________________________________

*/

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

