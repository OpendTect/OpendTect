#ifndef uivisslicepos3d_h
#define uivisslicepos3d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          April 2009
 RCS:           $Id: uivisslicepos3d.h,v 1.8 2011-10-10 08:34:56 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uislicepos.h"

namespace visSurvey { class PlaneDataDisplay; }


/*! \brief Toolbar for setting slice position _ 3D visualization display */

mClass uiSlicePos3DDisp : public uiSlicePos
{
public:		
			uiSlicePos3DDisp(uiParent*);

    void		setDisplay(visSurvey::PlaneDataDisplay*);
    int			getDisplayID() const;

protected:

    visSurvey::PlaneDataDisplay* curpdd_;
    void			slicePosChg(CallBacker*);
    void			sliceStepChg(CallBacker*);
    void			setBoxRanges();
    void			setPosBoxValue();
    void			setStepBoxValue();
};

#endif
