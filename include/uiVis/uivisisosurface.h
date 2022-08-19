#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

/*!\brief Dialog to set isovalue of an isosurface.  */

mExpClass(uiVis) uiVisIsoSurfaceThresholdDlg : public uiDlgGroup
{ mODTextTranslationClass(uiVisIsoSurfaceThresholdDlg);
public:
    		uiVisIsoSurfaceThresholdDlg(uiParent*,
			visBase::MarchingCubesSurface*,
			visSurvey::VolumeDisplay*, int attrib);
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
