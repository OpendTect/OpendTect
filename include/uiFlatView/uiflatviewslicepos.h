#ifndef uiflatviewslicepos_h
#define uiflatviewslicepos_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          April 2009
 RCS:           $Id: uiflatviewslicepos.h,v 1.3 2009-07-22 16:01:21 cvsbert Exp $
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
