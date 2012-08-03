#ifndef uivispolygonsurfbezierdlg_h
#define uivispolygonsurfbezierdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		Feb 2009
 RCS:		$Id: uivispolygonsurfbezierdlg.h,v 1.3 2012-08-03 13:01:19 cvskris Exp $
________________________________________________________________________


-*/

#include "uivismod.h"
#include "uidlggroup.h"

class uiGenInput;
class uiPushButton;
namespace Geometry { class PolygonSurface; }
namespace visSurvey { class PolygonBodyDisplay; };


mClass(uiVis) uiVisPolygonSurfBezierDlg : public uiDlgGroup
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

