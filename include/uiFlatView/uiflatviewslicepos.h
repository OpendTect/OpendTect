#ifndef uiflatviewslicepos_h
#define uiflatviewslicepos_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          April 2009
 RCS:           $Id: uiflatviewslicepos.h,v 1.1 2009-05-18 11:43:08 cvshelene Exp $
________________________________________________________________________

-*/

#include "uislicepos.h"

/*! \brief Toolbar for setting slice position _ 2D viewer */

mClass uiSlicePos2DView : public uiSlicePos
{
public:		
			uiSlicePos2DView(uiParent*);


protected:
    void		setBoxRanges();
    void		setPosBoxValue();
    void		setStepBoxValue();
    void		slicePosChg(CallBacker*);
    void		sliceStepChg(CallBacker*);

};

#endif
