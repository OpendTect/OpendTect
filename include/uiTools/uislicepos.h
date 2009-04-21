#ifndef uislicepos_h
#define uislicepos_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          April 2009
 RCS:           $Id: uislicepos.h,v 1.1 2009-04-21 09:55:20 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiparent.h"
#include "cubesampling.h"

class uiLabeledSpinBox;
class uiToolBar;

/*! \brief Toolbar for setting slice position _ base class */

mClass uiSlicePos : public CallBacker
{
public:		
			uiSlicePos(uiParent*);
			~uiSlicePos();

    uiToolBar*		getToolBar() const		{ return toolbar_; }
    CubeSampling	getCubeSampling() const		{ return curcs_; }

    enum Orientation            { Inline=0, Crossline=1, Timeslice=2 };
				DeclareEnumUtils(Orientation);

    Notifier<uiSlicePos> positionChg;

protected:

    uiToolBar*		toolbar_;
    uiLabeledSpinBox*	sliceposbox_;
    uiLabeledSpinBox*	slicestepbox_;
    int			laststeps_[3];
    CubeSampling	curcs_;

    void		setBoxLabel(Orientation);
    virtual void	setBoxRanges()			=0;
    virtual void	setPosBoxValue()		=0;
    virtual void	setStepBoxValue()		=0;
    virtual void	slicePosChg(CallBacker*)	=0;
    virtual void	sliceStepChg(CallBacker*)	=0;
    void		updatePos(CallBacker*);
    void		initSteps(CallBacker* cb=0);
    void		slicePosChanged(Orientation,const CubeSampling&);
    void		sliceStepChanged(Orientation);
    void		setBoxRg(Orientation,const CubeSampling&);
    void		setPosBoxVal(Orientation,const CubeSampling&);
};

#endif
