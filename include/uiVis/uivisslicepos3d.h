#ifndef uivisslicepos3d_h
#define uivisslicepos3d_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          April 2009
 RCS:           $Id: uivisslicepos3d.h,v 1.5 2009-04-21 09:55:20 cvshelene Exp $
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
