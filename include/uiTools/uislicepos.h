#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uiparent.h"
#include "bufstringset.h"
#include "trckeyzsampling.h"

class uiLabel;
class uiSpinBox;
class uiToolBar;
class uiToolButton;

/*! \brief Toolbar for setting slice position _ base class */

mExpClass(uiTools) uiSlicePos : public CallBacker
{ mODTextTranslationClass(uiSlicePos);
public:
			~uiSlicePos();

    typedef OD::SliceType SliceDir;
			mDeclareEnumUtils(SliceDir);

    uiToolBar*		getToolBar() const		{ return toolbar_; }
    TrcKeyZSampling	getTrcKeyZSampling() const	{ return curcs_; }

    void		setLabels(const uiString& inl,const uiString& crl,
				  const uiString& z);
    int			getStep(SliceDir) const;
    void		setStep(SliceDir,int step);
    void		setSteps(int inl,int crl,float z);

    float		getZStep() const;
    void		setZStep(float);

    Notifier<uiSlicePos> positionChg;

protected:
			uiSlicePos(uiParent*);

    uiToolBar*		toolbar_;
    uiLabel*		label_;
    uiSpinBox*		sliceposbox_;
    uiSpinBox*		slicestepbox_;
    uiToolButton*	prevbut_;
    uiToolButton*	nextbut_;
    float		laststeps_[3];
    int			zfactor_;
    TrcKeyZSampling	curcs_;
    uiStringSet		boxlabels_;

    virtual SliceDir	getOrientation() const		=0;
    void		setBoxLabel(SliceDir);
    virtual void	setBoxRanges()			=0;
    virtual void	setPosBoxValue()		=0;
    virtual void	setStepBoxValue()		=0;
    virtual void	slicePosChg(CallBacker*)	=0;
    virtual void	sliceStepChg(CallBacker*)	=0;
    void		shortcutsChg(CallBacker*);
    void		updatePos(CallBacker*);
    void		initSteps(CallBacker* cb=0);
    void		slicePosChanged(SliceDir,const TrcKeyZSampling&);
    void		sliceStepChanged(SliceDir);
    void		setBoxRg(SliceDir,const TrcKeyZSampling&,
				 const TrcKeyZSampling&);
    void		setPosBoxVal(SliceDir,const TrcKeyZSampling&);

    void		prevCB(CallBacker*);
    void		nextCB(CallBacker*);
};
