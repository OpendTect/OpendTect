#ifndef uivispolygonsurfbezierdlg_h
#define uivispolygonsurfbezierdlg_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		Feb 2009
 RCS:		$Id: uivispolygonsurfbezierdlg.h,v 1.1 2009-02-26 22:33:42 cvsyuancheng Exp $
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
