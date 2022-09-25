#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidlggroup.h"

class uiGenInput;
class uiPushButton;
namespace Geometry { class PolygonSurface; }
namespace visSurvey { class PolygonBodyDisplay; };


mExpClass(uiVis) uiVisPolygonSurfBezierDlg : public uiDlgGroup
{ mODTextTranslationClass(uiVisPolygonSurfBezierDlg);
public:
					uiVisPolygonSurfBezierDlg(uiParent*,
						visSurvey::PolygonBodyDisplay*);
					~uiVisPolygonSurfBezierDlg();

    bool				acceptOK();

protected:
    void				applyCB(CallBacker*);
    bool				apply();

    visSurvey::PolygonBodyDisplay*	plg_;
    Geometry::PolygonSurface*		surf_;

    uiGenInput*		bezierfld_;
    uiPushButton*			applybut_;
};
