#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "uidialog.h"
#include "uigroup.h"
#include "trckeyzsampling.h"
#include "ranges.h"
#include "threadlock.h"

class uiGenInput;
class uiLabeledSpinBox;
class uiScrollDialog;
class uiSliceScroll;
class uiSlider;
class uiSpinBox;
class uiToolButton;
namespace ZDomain { class Info; }

mExpClass(uiTools) uiSliceSel : public uiGroup
{ mODTextTranslationClass(uiSliceSel);
public:

    enum Type			{ Inl, Crl, Tsl, Vol, TwoD, Synth };
    enum class AutoScrollType	{ Stop=0, ContRev=1, Wrap=2 };

				uiSliceSel(uiParent*,Type,const ZDomain::Info&,
					const Pos::GeomID&,bool doscroll=false,
					const ZDomain::Info* dispzdinf=nullptr);
				~uiSliceSel();

    Type			getType() const { return type_; }
    bool			isInl() const	{ return type_ == Inl; }
    bool			isCrl() const	{ return type_ == Crl; }
    bool			isZSlice() const { return type_ == Tsl; }
    bool			isVol() const	{ return type_ == Vol; }
    bool			is2D() const	{ return type_ == TwoD; }
    bool			isSynth() const { return type_ == Synth; }
    bool			useTrcNr() const;
    bool			is3DSlice() const;
    bool			is2DSlice() const;
    bool			isSliderActive() const;

    void			setApplyCB(const CallBack&);

    const TrcKeyZSampling&	getTrcKeyZSampling() const { return tkzs_; }

    virtual void		setTrcKeyZSampling(const TrcKeyZSampling&);
    void			setMaxTrcKeyZSampling(const TrcKeyZSampling&);
    void			enableApplyButton(bool);
    void			fillPar(IOPar&);
    void			usePar(const IOPar&);
    void			stopAuto();
    void			disableAutoScroll(bool);

    bool			acceptOK();
    static uiString		sButTxtAdvance();
    static uiString		sButTxtPause();

    static Type			getType(const TrcKeyZSampling&);
    static bool			is3DSlice(Type);
    static bool			is2DSlice(Type);

    Notifier<uiSliceSel>	sliderMoved;

protected:

    void			createInlFld();
    void			createCrlFld();
    void			createZFld();

    void			initGrp(CallBacker*);
    void			fullPush(CallBacker*);
    void			applyPush(CallBacker*);

    void			prevCB(CallBacker*);
    void			nextCB(CallBacker*);
    void			settingsCB(CallBacker*);
    void			sliderMovedCB(CallBacker*);
    void			sliderReleasedCB(CallBacker*);
    void			playRevCB(CallBacker*);
    void			playPauseCB(CallBacker*);
    void			playForwardCB(CallBacker*);
    void			timerTickCB(CallBacker*);

    void			sliderValChanged();
    void			doNext(int step);
    void			doPrevious(int step);
    void			doMove(int step);
    void			doAuto();
    void			setTimer();

    void			readInput();
    void			updateUI();
    void			setBoxValues(uiSpinBox*,
					     const StepInterval<int>&,int);

    const ZDomain::Info&	zDomain(bool fordisplay) const;
    float			userFactor();
    int				nrDec();

    uiLabeledSpinBox*		inl0fld_			= nullptr;
    uiLabeledSpinBox*		crl0fld_;
    uiLabeledSpinBox*		z0fld_;
    uiSpinBox*			inl1fld_			= nullptr;
    uiSpinBox*			crl1fld_;
    uiSpinBox*			z1fld_;
    uiButton*			applybut_			= nullptr;

    uiGroup*			scrollgrp_			= nullptr;
    uiSlider*			slider_				= nullptr;
    uiToolButton*		playrevbut_			= nullptr;
    uiToolButton*		playpausebut_			= nullptr;
    uiToolButton*		playforwardbut_			= nullptr;
    uiGroup*			posgrp_				= nullptr;

    TrcKeyZSampling		maxcs_;
    TrcKeyZSampling		tkzs_;
    CallBack*			applycb_			= nullptr;
    Type			type_;
    bool			dogeomcheck_;
    const ZDomain::Info&	zdominfo_;
    const ZDomain::Info&	dispzdominfo_;

    struct AutoScroll
    {
	int			step_			= mUdf(int);
	float			dt_			= 0.2;
	bool			isforward_		= true;
	bool			autoon_			= false;
	bool			enabled_		= true;
	AutoScrollType		astype_			= AutoScrollType::Stop;
    };

    PtrMan<Timer>		timer_				= nullptr;
    AutoScroll			asprops_;
    bool			slideractive_			= false;

    Threads::Lock		updatelock_;
};


mExpClass(uiTools) uiSliceSelDlg : public uiDialog
{ mODTextTranslationClass(uiSliceSelDlg);
public:
				uiSliceSelDlg(uiParent*,
					      const TrcKeyZSampling& csin,
					      const TrcKeyZSampling& maxcs,
					      const CallBack& applycb,
					      uiSliceSel::Type,
					      const ZDomain::Info&,
					      bool withscroll=false);
				~uiSliceSelDlg();

    const TrcKeyZSampling&	getTrcKeyZSampling() const
				{ return slicesel_->getTrcKeyZSampling(); }
    void			setTrcKeyZSampling( const TrcKeyZSampling& cs )
				{ slicesel_->setTrcKeyZSampling( cs ); }

    uiSliceSel*			grp()	{ return slicesel_; }

protected:

    uiSliceSel*			slicesel_;

    bool			acceptOK(CallBacker*) override;
    bool			rejectOK(CallBacker*) override;
};


mExpClass(uiTools) uiLinePosSelDlg : public uiDialog
{ mODTextTranslationClass(uiLinePosSelDlg);
public:
			uiLinePosSelDlg(uiParent*);
			uiLinePosSelDlg(uiParent*,const TrcKeyZSampling&);
			~uiLinePosSelDlg();

    const TrcKeyZSampling& getTrcKeyZSampling() const;
    const char*		getLineName() const;
    void		setPrefCS( TrcKeyZSampling* tkzs )
			{ prefcs_ = tkzs; }

protected:
    bool		acceptOK(CallBacker*) override;
    bool		selectPos2D();
    bool		selectPos3D();

    uiGenInput*		inlcrlfld_ = nullptr;
    uiGenInput*		linesfld_ = nullptr;
    TrcKeyZSampling	tkzs_;
    TrcKeyZSampling*	prefcs_ = nullptr;
    uiSliceSelDlg*	posdlg_ = nullptr;
    IOPar		prevpar_;
};
