#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          April 2009
________________________________________________________________________

-*/

#include "uivismod.h"

#include "uislicepos.h"
#include "visplanedatadisplay.h"
#include "visvolumedisplay.h"

class uiVisPartServer;

/*! \brief Toolbar for setting slice position _ 3D visualization display */

mExpClass(uiVis) uiSlicePos3DDisp : public uiSlicePos
{
public:
			uiSlicePos3DDisp(uiParent*,uiVisPartServer*);

    void		setDisplay(VisID dispid);
    VisID		getDisplayID() const;

private:

    RefMan<visSurvey::PlaneDataDisplay> curpdd_;
    RefMan<visSurvey::VolumeDisplay>	curvol_;
    uiVisPartServer*		vispartserv_;

    uiSlicePos::SliceDir	getOrientation() const;
    TrcKeyZSampling		getSampling() const;

    void			slicePosChg(CallBacker*);
    void			sliceStepChg(CallBacker*);
    void			setBoxRanges();
    void			setPosBoxValue();
    void			setStepBoxValue();
};

