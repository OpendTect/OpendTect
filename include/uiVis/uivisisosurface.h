#ifndef uivisisosurface_h
#define uivisisosurface_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		24-01-2003
 RCS:		$Id: uivisisosurface.h,v 1.2 2007-03-28 12:20:46 cvsbert Exp $
________________________________________________________________________


-*/

#include "uidlggroup.h"

class uiCanvas;
class uiHistogramDisplay;
class uiGenInput;
class uiPushButton;
namespace visBase { class IsoSurface; }
template <class T> class SamplingData;
template <class T> class TypeSet;

/*!\brief Dialog to set isovalue of an isosurface.  */

class uiVisIsoSurfaceThresholdDlg : public uiDlgGroup
{
public:
    		uiVisIsoSurfaceThresholdDlg(uiParent*,visBase::IsoSurface&,
					    const TypeSet<float>& histogram,
					    const SamplingData<float>& xaxis);
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

    uiCanvas*		histogramdisplay_;
    uiHistogramDisplay*	histogrampainter_;
    uiGenInput*		thresholdfld_;
    uiPushButton*	updatebutton_;

    visBase::IsoSurface& isosurfacedisplay_;
    float		initialvalue_;

};

#endif
