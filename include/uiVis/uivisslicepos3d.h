#ifndef uislicepos_h
#define uislicepos_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2006
 RCS:           $Id: uivisslicepos3d.h,v 1.4 2009-01-08 10:37:54 cvsranojay Exp $
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

mClass uiSlicePos : public CallBacker
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
    void		initSteps(CallBacker* cb=0);
};

#endif
