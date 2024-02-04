#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"

#include "uigroup.h"
#include "trckeyzsampling.h"

class uiCheckBox;
class uiSpinBox;
class uiLabel;
class uiLabeledSpinBox;
class uiLineEdit;
namespace ZDomain { class Info; }

/*!\brief Selects sub-Z-range. Default will be SI() work Z Range. */

mExpClass(uiIo) uiSelZRange : public uiGroup
{ mODTextTranslationClass(uiSelZRange)
public:
			uiSelZRange(uiParent*,bool wstep,
				    const char* zdomkey=nullptr,
				    const char* zunitstr=nullptr);
    mDeprecatedObs	uiSelZRange(uiParent*,bool wstep,
				    bool isrel,const char* lbltxt=nullptr,
				    const char* zdomkey=nullptr,
				    const char* zunitstr=nullptr);
    mDeprecatedObs	uiSelZRange(uiParent* p,const ZSampling& limitrg,
				    bool wstep,const char* lbltxt=nullptr,
				    const char* zdomkey=nullptr,
				    const char* zunitstr=nullptr);
			~uiSelZRange();

    void		setLabel(const uiString&);
    void		setRange(const ZSampling&);
    void		setRangeLimits(const ZSampling&);
    void		setIsRelative(bool yn=true);

    ZSampling		getRange() const;
    const ZDomain::Info& zDomain() const	{ return *zinfo_; }
    bool		isSIDomain() const;
    void		displayStep(bool yn);

    Notifier<uiSelZRange> rangeChanged;

protected:

    void		makeInpFields(bool withstep);
    bool		canSnap() const;
    void		valChg(CallBacker*);

    uiLabel*		lblfld_;
    uiSpinBox*		startfld_;
    uiSpinBox*		stopfld_;
    uiLabeledSpinBox*	stepfld_ = nullptr;
    bool		isrel_ = false;
    const ZDomain::Info* zinfo_;

};


/*!\brief Selects range of trace numbers */

mExpClass(uiIo) uiSelNrRange : public uiGroup
{ mODTextTranslationClass(uiSelNrRange)
public:
    enum Type		{ Inl, Crl, Gen };

			uiSelNrRange(uiParent*,Type,bool wstep);
			uiSelNrRange(uiParent*,bool wstep,const uiString& lbl,
				     const StepInterval<int>* limit=nullptr);
    mDeprecated("Use uiString")
			uiSelNrRange(uiParent*,const StepInterval<int>& limit,
				     bool wstep,const char* lbltxt);
			~uiSelNrRange();

    StepInterval<int>	getRange() const;
    void		setRange(const StepInterval<int>&);
    void		setLimitRange(const StepInterval<int>&);
    void		displayStep( bool yn );

    bool		isChecked();
    void		setChecked(bool);
    bool		isCheckable()		{ return cbox_; }
    void		setWithCheck( bool yn=true )	{ withchk_ = yn; }

    Notifier<uiSelNrRange>	checked;
    Notifier<uiSelNrRange>	rangeChanged;

protected:

    uiCheckBox*		cbox_				= nullptr;
    uiSpinBox*		startfld_;
    uiSpinBox*		icstopfld_			= nullptr;
    uiLineEdit*		nrstopfld_			= nullptr;
    uiLabeledSpinBox*	stepfld_			= nullptr;
    uiString		lbltxt_;
    bool		finalized_			= false;
    bool		withchk_			= false;
    int			defstep_;

    void		valChg(CallBacker*);
    void		checkBoxSel(CallBacker*);
    void		doFinalize(CallBacker*);

    int			getStopVal() const;
    void		setStopVal(int);
    void		makeInpFields(StepInterval<int>,bool,bool);

private:
    bool		checked_			= false;

};


/*!\brief Selects step(s) in inl/crl or trcnrs */

mExpClass(uiIo) uiSelSteps : public uiGroup
{ mODTextTranslationClass(uiSelSteps)
public:
			uiSelSteps(uiParent*,bool is2d);
			~uiSelSteps();

    BinID		getSteps() const;
    void		setSteps(const BinID&);

protected:

    uiSpinBox*		inlfld_		= nullptr;
    uiSpinBox*		crlfld_;

};


/*!\brief Selects sub-volume. Default will be SI() work area */

mExpClass(uiIo) uiSelHRange : public uiGroup
{ mODTextTranslationClass(uiSelHRange)
public:
			uiSelHRange(uiParent*,bool wstep,
				    const TrcKeySampling* limiths=nullptr);
			~uiSelHRange();

    TrcKeySampling	getSampling() const;
    void		setSampling(const TrcKeySampling&);
    void		setLimits(const TrcKeySampling&);
    void		displayStep( bool yn );

    uiSelNrRange*	inlfld_;
    uiSelNrRange*	crlfld_;

};


/*!\brief Selects sub-volume. Default will be SI() work volume */

mExpClass(uiIo) uiSelSubvol : public uiGroup
{
public:
			uiSelSubvol(uiParent*,bool wstep,
				    const char* zdomkey=nullptr,
				    const char* zunitstr=nullptr);
			~uiSelSubvol();

    TrcKeyZSampling	getSampling() const;
    void		setSampling(const TrcKeyZSampling&);

    uiSelHRange*	hfld_;
    uiSelZRange*	zfld_;

};
