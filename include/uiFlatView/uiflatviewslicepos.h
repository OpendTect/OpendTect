#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          April 2009
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "uislicepos.h"
#include "zdomain.h"

/*!
\brief Toolbar for setting slice position _ 2D viewer.
*/

mExpClass(uiFlatView) uiSlicePos2DView : public uiSlicePos
{
public:
			uiSlicePos2DView(uiParent*,const ZDomain::Info&);

    void		setTrcKeyZSampling(const TrcKeyZSampling&);
    void		setLimitSampling(const TrcKeyZSampling&);

    const TrcKeyZSampling& getLimitSampling() const { return limitscs_; };

protected:

    void		setBoxRanges() override;
    void		setPosBoxValue() override;
    void		setStepBoxValue() override;
    void		handleSlicePosChg() override;
    void		handleSliceStepChg() override;

    TrcKeyZSampling	limitscs_;
    SliceDir		curorientation_;
    ZDomain::Info	zdomaininfo_;

};
