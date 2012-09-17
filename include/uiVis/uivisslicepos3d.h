#ifndef uivisslicepos3d_h
#define uivisslicepos3d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          April 2009
 RCS:           $Id: uivisslicepos3d.h,v 1.9 2012/01/02 14:04:14 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uislicepos.h"

namespace visSurvey 
{ 
    class PlaneDataDisplay; 
    class VolumeDisplay; 
    class SurveyObject; 
}
class uiVisPartServer;

/*! \brief Toolbar for setting slice position _ 3D visualization display */

mClass uiSlicePos3DDisp : public uiSlicePos
{
public:		
			uiSlicePos3DDisp(uiParent*,uiVisPartServer*);

    void		setDisplay(int dispid);
    int			getDisplayID() const;

protected:

    visSurvey::PlaneDataDisplay* curpdd_;
    visSurvey::VolumeDisplay* 	curvol_;
    uiVisPartServer*		vispartserv_;

    uiSlicePos::Orientation 	getOrientation() const;
    CubeSampling		getSampling() const;

    void			slicePosChg(CallBacker*);
    void			sliceStepChg(CallBacker*);
    void			setBoxRanges();
    void			setPosBoxValue();
    void			setStepBoxValue();
};

#endif
