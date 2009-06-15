#ifndef uiflatviewslicepos_h
#define uiflatviewslicepos_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          April 2009
 RCS:           $Id: uiflatviewslicepos.h,v 1.2 2009-06-15 12:17:23 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uislicepos.h"

/*! \brief Toolbar for setting slice position _ 2D viewer */

mClass uiSlicePos2DView : public uiSlicePos
{
public:		
			uiSlicePos2DView(uiParent*);

    void		setCubeSampling(const CubeSampling&);

protected:
    void		setBoxRanges();
    void		setPosBoxValue();
    void		setStepBoxValue();
    void		slicePosChg(CallBacker*);
    void		sliceStepChg(CallBacker*);

    CubeSampling	curcs_;
    Orientation		curorientation_;
};

#endif
