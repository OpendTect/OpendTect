#ifndef uivisisosurface_h
#define uivisisosurface_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		24-01-2003
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uivismod.h"
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

mClass(uiVis) uiVisIsoSurfaceThresholdDlg : public uiDlgGroup
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
    uiIOObjSel*		seedselfld_;
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

