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
#include "zdomain.h"

class uiGenInput;
class uiLabeledSpinBox;
class uiScrollDialog;
class uiSliceScroll;
class uiSpinBox;

mExpClass(uiTools) uiSliceSel : public uiGroup
{ mODTextTranslationClass(uiSliceSel);
public:

    enum Type			{ Inl, Crl, Tsl, Vol, TwoD };

				uiSliceSel(uiParent*,Type,const ZDomain::Info&,
					   bool dogeomcheck=true);
				~uiSliceSel();

    void			setApplyCB(const CallBack&);

    const TrcKeyZSampling&		getTrcKeyZSampling() const { 
								   return tkzs_;
								   }
    virtual void		setTrcKeyZSampling(const TrcKeyZSampling&);
    void			setMaxTrcKeyZSampling(const TrcKeyZSampling&);
    void			enableApplyButton(bool);
    void			enableScrollButton(bool);
    void			fillPar(IOPar&);
    void			usePar(const IOPar&);

    bool			acceptOK();
    static uiString		sButTxtAdvance();
    static uiString		sButTxtPause();

protected:

    friend class		uiSliceScroll;

    void			createInlFld();
    void			createCrlFld();
    void			createZFld();

    void			fullPush(CallBacker*);
    void			scrollPush(CallBacker*);
    void			applyPush(CallBacker*);
    void			readInput();
    void			updateUI();
    void			setBoxValues(uiSpinBox*,
					     const StepInterval<int>&,int);

    uiLabeledSpinBox*		inl0fld_;
    uiLabeledSpinBox*		crl0fld_;
    uiLabeledSpinBox*		z0fld_;
    uiSpinBox*			inl1fld_;
    uiSpinBox*			crl1fld_;
    uiSpinBox*			z1fld_;
    uiButton*			applybut_;
    uiButton*			scrollbut_;

    uiSliceScroll*		scrolldlg_;

    TrcKeyZSampling		maxcs_;
    TrcKeyZSampling		tkzs_;
    CallBack*			applycb_;
    bool			isinl_, iscrl_, istsl_, isvol_, is2d_,
				dogeomcheck_;
    ZDomain::Info		zdominfo_;

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
					      const ZDomain::Info&);
				~uiSliceSelDlg();

    const TrcKeyZSampling&	getTrcKeyZSampling() const
				{ return slicesel_->getTrcKeyZSampling(); }
    void			setTrcKeyZSampling( const TrcKeyZSampling& cs )
				{ slicesel_->setTrcKeyZSampling( cs ); }

    uiSliceSel*			grp()	{ return slicesel_; }

protected:

    uiSliceSel*			slicesel_;

    bool			acceptOK(CallBacker*) override;
};


mExpClass(uiTools) uiLinePosSelDlg : public uiDialog
{ mODTextTranslationClass(uiLinePosSelDlg);
public:
			uiLinePosSelDlg(uiParent*);
			uiLinePosSelDlg(uiParent*,const TrcKeyZSampling&);
			~uiLinePosSelDlg();

    const TrcKeyZSampling&	getTrcKeyZSampling() const;
    const char*		getLineName() const;
    void		setPrefCS(TrcKeyZSampling* prefcs) { 
							    prefcs_ = prefcs;
							   }

protected:
    bool		acceptOK(CallBacker*) override;
    bool		selectPos2D();
    bool		selectPos3D();

    uiGenInput*		inlcrlfld_;
    uiGenInput*		linesfld_;
    TrcKeyZSampling	tkzs_;
    TrcKeyZSampling*	prefcs_;
    uiSliceSelDlg*	posdlg_;
    bool		is2d_;
    IOPar		prevpar_;
};
