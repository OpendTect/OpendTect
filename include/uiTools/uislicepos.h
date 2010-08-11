#ifndef uislicepos_h
#define uislicepos_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          April 2009
 RCS:           $Id: uislicepos.h,v 1.5 2010-08-11 09:55:38 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiparent.h"
#include "cubesampling.h"

class uiLabeledSpinBox;
class uiToolBar;
class uiToolButton;

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
    uiToolButton*	prevbut_;
    uiToolButton*	nextbut_;
    int			laststeps_[3];
    float		zfactor_;
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

    void		prevCB(CallBacker*);
    void		nextCB(CallBacker*);
};

#endif
