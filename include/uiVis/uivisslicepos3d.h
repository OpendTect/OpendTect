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

namespace visSurvey
{
    class PlaneDataDisplay;
    class VolumeDisplay;
    class SurveyObject;
}
class uiVisPartServer;

/*! \brief Toolbar for setting slice position _ 3D visualization display */

mExpClass(uiVis) uiSlicePos3DDisp : public uiSlicePos
{
public:
			uiSlicePos3DDisp(uiParent*,uiVisPartServer*);

    void		setDisplay(int dispid);
    int			getDisplayID() const;

protected:

    visSurvey::PlaneDataDisplay* curpdd_;
    visSurvey::VolumeDisplay*	curvol_;
    uiVisPartServer*		vispartserv_;

    uiSlicePos::SliceDir	getOrientation() const;
    TrcKeyZSampling		getSampling() const;

    void			handleSlicePosChg() override;
    void			handleSliceStepChg() override;
    void			setBoxRanges() override;
    void			setPosBoxValue() override;
    void			setStepBoxValue() override;

};
