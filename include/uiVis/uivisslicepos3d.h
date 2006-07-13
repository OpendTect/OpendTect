#ifndef uislicepos_h
#define uislicepos_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2006
 RCS:           $Id: uivisslicepos3d.h,v 1.1 2006-07-13 20:18:51 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiparent.h"
#include "cubesampling.h"

namespace visSurvey { class PlaneDataDisplay; }

class uiSpinBox;
class uiToolBar;

/*! \brief Toolbar for setting slice position
*/

class uiSlicePos : public CallBacker
{
public:		
			uiSlicePos(uiParent*);
			~uiSlicePos();

    uiToolBar*		getToolBar() const		{ return toolbar_; }
    void		setDisplay(visSurvey::PlaneDataDisplay*);

protected:

    uiToolBar*		toolbar_;
    uiSpinBox*		sliceposbox_;
    uiSpinBox*		slicestepbox_;
    visSurvey::PlaneDataDisplay* curpdd_;
    int			laststeps_[3];

    void		setBoxRanges();
    void		setPosBoxValue();
    void		setStepBoxValue();
    void		slicePosChg(CallBacker*);
    void		sliceStepChg(CallBacker*);
    void		updatePos(CallBacker*);
};

#endif
