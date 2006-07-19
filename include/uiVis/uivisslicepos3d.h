#ifndef uislicepos_h
#define uislicepos_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2006
 RCS:           $Id: uivisslicepos3d.h,v 1.2 2006-07-19 15:21:25 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiparent.h"
#include "cubesampling.h"

namespace visSurvey { class PlaneDataDisplay; }

class uiLabeledSpinBox;
class uiToolBar;
class uiVisPartServer;

/*! \brief Toolbar for setting slice position
*/

class uiSlicePos : public CallBacker
{
public:		
			uiSlicePos(uiParent*);
			~uiSlicePos();

    uiToolBar*		getToolBar() const		{ return toolbar_; }
    void		setDisplay(visSurvey::PlaneDataDisplay*);
    int			getDisplayID() const;
    CubeSampling	getCubeSampling() const		{ return curcs_; }

    Notifier<uiSlicePos> positionChg;

protected:

    uiToolBar*		toolbar_;
    uiLabeledSpinBox*	sliceposbox_;
    uiLabeledSpinBox*	slicestepbox_;
    visSurvey::PlaneDataDisplay* curpdd_;
    int			laststeps_[3];
    CubeSampling	curcs_;

    void		setBoxLabel();
    void		setBoxRanges();
    void		setPosBoxValue();
    void		setStepBoxValue();
    void		slicePosChg(CallBacker*);
    void		sliceStepChg(CallBacker*);
    void		updatePos(CallBacker*);
};

#endif
