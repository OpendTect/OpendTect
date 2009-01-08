#ifndef uivisisosurface_h
#define uivisisosurface_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		24-01-2003
 RCS:		$Id: uivisisosurface.h,v 1.12 2009-01-08 10:37:54 cvsranojay Exp $
________________________________________________________________________


-*/

#include "uidlggroup.h"

class uiAxisHandler;
class uiHistogramDisplay;
class uiGenInput;
class uiIOObjSel;
class uiLineItem;
class uiPushButton;
class uiStatsDisplay;

namespace visBase { class MarchingCubesSurface; }
namespace visSurvey { class VolumeDisplay; }
template <class T> class SamplingData;
template <class T> class TypeSet;

/*!\brief Dialog to set isovalue of an isosurface.  */

mClass uiVisIsoSurfaceThresholdDlg : public uiDlgGroup
{
public:
    		uiVisIsoSurfaceThresholdDlg(uiParent*,
			visBase::MarchingCubesSurface*,
			visSurvey::VolumeDisplay*);
		~uiVisIsoSurfaceThresholdDlg();

    bool	acceptOK();
    bool	rejectOK();
    bool	revertChanges();

protected:

    void		reDrawCB(CallBacker*);
    void		updatePressed(CallBacker*);
    void		mousePressed(CallBacker*);
    void		modeChangeCB(CallBacker*);
    void		doubleClick(CallBacker*);
    void		handleClick(CallBacker*,bool isdouble);
    void		updateIsoDisplay(float nv);
    void		drawHistogram();

    uiStatsDisplay*	statsdisplay_;
    uiGenInput*		modefld_;
    uiGenInput*		thresholdfld_;
    uiGenInput*		aboveisovaluefld_;
    uiIOObjSel*		ioobjselfld_;
    uiPushButton*	updatebutton_;

    uiLineItem*		initiallineitem_;
    uiLineItem*		thresholdlineitem_;
    uiLineItem*		isovallineitem_;

    visBase::MarchingCubesSurface*	isosurfacedisplay_;
    visSurvey::VolumeDisplay*		vd_;
    float				initialvalue_;

    uiAxisHandler&	xAxis();
    uiHistogramDisplay&	funcDisp();
};

#endif
