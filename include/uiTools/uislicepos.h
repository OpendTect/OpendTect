#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "uiparent.h"
#include "odcommonenums.h"
#include "trckeyzsampling.h"

class uiLabel;
class uiSlider;
class uiSpinBox;
class uiToolBar;
class uiToolButton;

namespace ZDomain { class Info; }

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
    bool		isSliderActive() const;

    Notifier<uiSlicePos> positionChg;
    Notifier<uiSlicePos> sliderReleased;

protected:
			uiSlicePos(uiParent*);

    uiToolBar*		toolbar_;
    uiLabel*		label_;
    uiSpinBox*		sliceposbox_;
    uiSpinBox*		slicestepbox_;
    uiToolButton*	prevbut_;
    uiToolButton*	nextbut_;
    uiSlider*		sliceslider_;
    float		laststeps_[3];
    float		zfactor_	= mUdf(float);
    TrcKeyZSampling	curcs_;
    uiStringSet		boxlabels_;

    bool		isslideractive_			= false;

    const ZDomain::Info*    zdominfo_			= nullptr;
    const ZDomain::Info*    dispzdominfo_		= nullptr;

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

    void		sliderSlicePosChg(CallBacker*);
    void		sliderReleasedCB(CallBacker*);
    void		prevCB(CallBacker*);
    void		nextCB(CallBacker*);
};
