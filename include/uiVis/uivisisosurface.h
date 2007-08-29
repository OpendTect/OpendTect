#ifndef uivisisosurface_h
#define uivisisosurface_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		24-01-2003
 RCS:		$Id: uivisisosurface.h,v 1.3 2007-08-29 14:25:51 cvskris Exp $
________________________________________________________________________


-*/

#include "uidlggroup.h"

class uiCanvas;
class uiHistogramDisplay;
class uiGenInput;
class uiPushButton;
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

    uiCanvas*				histogramdisplay_;
    uiHistogramDisplay*			histogrampainter_;
    uiGenInput*				thresholdfld_;
    uiPushButton*			updatebutton_;

    visBase::MarchingCubesSurface*	isosurfacedisplay_;
    visSurvey::VolumeDisplay*		vd_;
    float				initialvalue_;

};

#endif
