#ifndef uivisisosurface_h
#define uivisisosurface_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		24-01-2003
 RCS:		$Id: uivisisosurface.h,v 1.4 2008-04-01 15:43:03 cvsbert Exp $
________________________________________________________________________


-*/

#include "uidlggroup.h"

class uiStatsDisplay;
class uiGenInput;
class uiPushButton;
class uiAxisHandler;
class uiFunctionDisplay;
namespace visBase { class MarchingCubesSurface; }
namespace visSurvey { class VolumeDisplay; }
template <class T> class SamplingData;
template <class T> class TypeSet;

/*!\brief Dialog to set isovalue of an isosurface.  */

class uiVisIsoSurfaceThresholdDlg : public uiDlgGroup
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

    void		updatePressed(CallBacker*);
    void		mousePressed(CallBacker*);
    void		doubleClick(CallBacker*);
    void		handleClick(CallBacker*,bool isdouble );
    void		updateIsoDisplay(float nv);
    void		updateHistogramDisplay(CallBacker* = 0);

    uiStatsDisplay*	statsdisplay_;
    uiGenInput*		thresholdfld_;
    uiPushButton*	updatebutton_;

    visBase::MarchingCubesSurface*	isosurfacedisplay_;
    visSurvey::VolumeDisplay*		vd_;
    float				initialvalue_;

    uiAxisHandler&	xAxis();
    uiFunctionDisplay&	funcDisp();
};

#endif
