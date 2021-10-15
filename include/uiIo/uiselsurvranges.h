#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uigroup.h"
#include "trckeyzsampling.h"

class uiCheckBox;
class uiSpinBox;
class uiLabeledSpinBox;
class uiLineEdit;
namespace ZDomain { class Def; }

/*!\brief Selects sub-Z-range. Default will be SI() work Z Range. */

mExpClass(uiIo) uiSelZRange : public uiGroup
{ mODTextTranslationClass(uiSelZRange)
public:
                        uiSelZRange(uiParent*,bool wstep,
				    bool isrel=false,const char* lbltxt=0,
				    const char* zdomkey=0);
			uiSelZRange(uiParent* p,StepInterval<float> limitrg,
				    bool wstep,const char* lbltxt=0,
				    const char* zdomkey=0);

    StepInterval<float>	getRange() const;
    void		setRange(const StepInterval<float>&);
    void		setRangeLimits(const StepInterval<float>&);
    void		displayStep( bool yn );

    Notifier<uiSelZRange>	rangeChanged;

    const ZDomain::Def&	zDomainDef() const	{ return zddef_; }

protected:

    uiSpinBox*		startfld_;
    uiSpinBox*		stopfld_;
    uiLabeledSpinBox*	stepfld_;
    bool		isrel_;
    const ZDomain::Def&	zddef_; // keep above othdom_.
    const bool		othdom_; // keep above cansnap_
    const bool		cansnap_;

    void		valChg(CallBacker*);
    void		makeInpFields(const uiString&,bool,
	    			      const StepInterval<float>*);

};


/*!\brief Selects range of trace numbers */

mExpClass(uiIo) uiSelNrRange : public uiGroup
{
public:
    enum Type		{ Inl, Crl, Gen };

                        uiSelNrRange(uiParent*,Type,bool wstep);
			uiSelNrRange(uiParent*,StepInterval<int> limit,
				     bool wstep,const char*);

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

    uiCheckBox*		cbox_;
    uiSpinBox*		startfld_;
    uiSpinBox*		icstopfld_;
    uiLineEdit*		nrstopfld_;
    uiLabeledSpinBox*	stepfld_;
    BufferString	lbltxt_;
    bool		finalised_;
    bool		withchk_;
    int			defstep_;

    void		valChg(CallBacker*);
    void		checkBoxSel(CallBacker*);
    void		doFinalise(CallBacker*);

    int			getStopVal() const;
    void		setStopVal(int);
    void		makeInpFields(StepInterval<int>,bool,bool);

private:
    bool		checked_;

};


/*!\brief Selects step(s) in inl/crl or trcnrs */

mExpClass(uiIo) uiSelSteps : public uiGroup
{ mODTextTranslationClass(uiSelSteps)
public:

                        uiSelSteps(uiParent*,bool is2d);

    BinID		getSteps() const;
    void		setSteps(const BinID&);

protected:

    uiSpinBox*		inlfld_;
    uiSpinBox*		crlfld_;

};


/*!\brief Selects sub-volume. Default will be SI() work area */

mExpClass(uiIo) uiSelHRange : public uiGroup
{ mODTextTranslationClass(uiSelHRange)
public:
                        uiSelHRange(uiParent*,bool wstep);
			uiSelHRange(uiParent*,const TrcKeySampling& limiths,
				    bool wstep);

    TrcKeySampling		getSampling() const;
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
                        uiSelSubvol(uiParent*,bool wstep,const char* zdomkey=0);

    TrcKeyZSampling	getSampling() const;
    void		setSampling(const TrcKeyZSampling&);

    uiSelHRange*	hfld_;
    uiSelZRange*	zfld_;

};



