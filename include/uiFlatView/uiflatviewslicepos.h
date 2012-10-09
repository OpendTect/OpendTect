#ifndef uiflatviewslicepos_h
#define uiflatviewslicepos_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          April 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uislicepos.h"

/*! \brief Toolbar for setting slice position _ 2D viewer */

mClass uiSlicePos2DView : public uiSlicePos
{
public:		
			uiSlicePos2DView(uiParent*);

    void		setCubeSampling(const CubeSampling&);
    void		setLimitSampling(const CubeSampling&);

    const CubeSampling& getLimitSampling() const { return limitscs_; };

protected:
    void		setBoxRanges();
    void		setPosBoxValue();
    void		setStepBoxValue();
    void		slicePosChg(CallBacker*);
    void		sliceStepChg(CallBacker*);

    CubeSampling	limitscs_;
    Orientation		curorientation_;
};

#endif
