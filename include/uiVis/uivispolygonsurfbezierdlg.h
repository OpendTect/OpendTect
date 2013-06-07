#ifndef uivispolygonsurfbezierdlg_h
#define uivispolygonsurfbezierdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		Feb 2009
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uidlggroup.h"

class uiGenInput;
class uiPushButton;
namespace Geometry { class PolygonSurface; }
namespace visSurvey { class PolygonBodyDisplay; };


mClass uiVisPolygonSurfBezierDlg : public uiDlgGroup
{
public:
    					uiVisPolygonSurfBezierDlg(uiParent*,
						visSurvey::PolygonBodyDisplay*);
    bool				acceptOK();
protected:
    bool				applyCB(CallBacker*);

    visSurvey::PolygonBodyDisplay*	plg_;
    Geometry::PolygonSurface*		surf_;

    uiGenInput*         		bezierfld_;
    uiPushButton*			applybut_;
};


#endif
