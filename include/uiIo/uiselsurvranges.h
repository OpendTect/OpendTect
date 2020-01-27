#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uigroup.h"

#include "geomid.h"

class CubeHorSubSel;
class CubeSubSel;
class LineSubSel;
class LineSubSelSet;
class uiCheckBox;
class uiGenInput;
class uiLabeledSpinBox;
class uiLineEdit;
class uiSpinBox;
namespace ZDomain { class Def; }


mExpClass(uiIo) uiSelRangeBase : public uiGroup
{
public:
    typedef Pos::TraceNr_Type		trcnr_type;
    typedef StepInterval<trcnr_type>	trcnr_steprg_type;

    virtual		~uiSelRangeBase();

    virtual void	displayStep(bool yn)			= 0;

    Notifier<uiSelRangeBase>	rangeChanged;

protected:
			uiSelRangeBase(uiParent*,const char* grpnm);

public:

    virtual void	valChg(CallBacker*);

};


/*!\brief Selects sub-Z-range. Default will be SI() work Z Range. */

mExpClass(uiIo) uiSelZRange : public uiSelRangeBase
{ mODTextTranslationClass(uiSelZRange)
public:
                        uiSelZRange(uiParent*,bool wstep,
				    bool isrel=false,
				    const uiString& seltxt=uiString(),
				    const char* zdomkey=0);
			uiSelZRange(uiParent* p,ZSampling limitrg,bool wstep,
				    const uiString& seltxt=uiString(),
				    const char* zdomkey=0);
			~uiSelZRange();

    ZSampling		getRange() const;
    void		setRange(const ZSampling&);
    void		setRangeLimits(const ZSampling&);
    void		displayStep(bool yn) override;

    const ZDomain::Def&	zDomainDef() const	{ return zddef_; }

protected:

    uiSpinBox*		startfld_;
    uiSpinBox*		stopfld_;
    uiLabeledSpinBox*	stepfld_;
    bool		isrel_;
    const ZDomain::Def&	zddef_; // keep above othdom_.
    const bool		othdom_; // keep above cansnap_
    const bool		cansnap_;

    void		makeInpFields(const uiString&,bool,const ZSampling*);
    void		valChg(CallBacker*) override;

};


/*!\brief Selects range of trace numbers */

mExpClass(uiIo) uiSelNrRange : public uiSelRangeBase
{ mODTextTranslationClass(uiSelNrRange)
public:
    enum Type		{ Inl, Crl, Trc, Gen };

                        uiSelNrRange(uiParent*,Type,bool wstep);
			uiSelNrRange(uiParent*,trcnr_steprg_type limit,
				     bool wstep,const char* fldnm);
			~uiSelNrRange();

    trcnr_steprg_type	getRange() const;
    void		setRange(const trcnr_steprg_type&);
    void		setLimitRange(const trcnr_steprg_type&);
    void		displayStep(bool yn) override;

    bool		isChecked();
    void		setChecked(bool);
    bool		isCheckable()		{ return cbox_; }
    void		setWithCheck( bool yn=true )	{ withchk_ = yn; }
    void		setLabelText( const uiString& t ) { lbltxt_ = t; }

    Notifier<uiSelNrRange>	checked;

protected:

    uiCheckBox*		cbox_ = nullptr;
    uiSpinBox*		startfld_;
    uiSpinBox*		icstopfld_ = nullptr;
    uiLineEdit*		nrstopfld_ = nullptr;
    uiLabeledSpinBox*	stepfld_ = nullptr;
    BufferString	fldnm_;
    uiString		lbltxt_;
    bool		finalised_;
    bool		withchk_;
    int			defstep_;

    void		valChg(CallBacker*) override;
    void		checkBoxSel(CallBacker*);
    void		doFinalise(CallBacker*);

    int			getStopVal() const;
    void		setStopVal(int);
    void		makeInpFields(trcnr_steprg_type,bool,bool);

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

    uiSpinBox*		inlfld_ = nullptr;
    uiSpinBox*		crlfld_;

};


/*!\brief Selects sub-volume. Default will be SI() work area */

mExpClass(uiIo) uiSelHRange : public uiSelRangeBase
{ mODTextTranslationClass(uiSelHRange)
public:
                        uiSelHRange(uiParent*,bool wstep);
			uiSelHRange(uiParent*,const CubeHorSubSel&,
				    bool wstep);
			~uiSelHRange();

    CubeHorSubSel	getSampling() const;
    void		setSampling(const CubeHorSubSel&);
    void		setLimits(const CubeHorSubSel&);
    void		displayStep(bool yn) override;

    uiSelNrRange*	inlfld_;
    uiSelNrRange*	crlfld_;

};


/*!\brief Selects sub-volume. Default will be SI() work volume */

mExpClass(uiIo) uiSelSubvol : public uiSelRangeBase
{ mODTextTranslationClass(uiSelSubvol)
public:
			uiSelSubvol(uiParent*,bool wstep=true,
				    bool withz=true,const char* zdomkey=0,
				    const CubeSubSel* = nullptr);
			~uiSelSubvol();

    bool		hasZ() const		{ return zfld_; }
    CubeSubSel		getSampling() const;
    ZSampling		getZRange() const;

    void		setSampling(const CubeSubSel&);
    void		displayStep(bool yn) override;

private:

    uiSelHRange*	hfld_;
    uiSelZRange*	zfld_ = nullptr;

};


/*!\brief Selects sub-line. Default will be 1-udf and SI() z range */

mExpClass(uiIo) uiSelSubline : public uiSelRangeBase
{ mODTextTranslationClass(uiSelSubline)
public:
    mUseType( uiSelRangeBase, trcnr_steprg_type );

			uiSelSubline(uiParent*,Pos::GeomID,
				     bool wstep=true,bool withz=true);
			~uiSelSubline();

    bool		hasZ() const		{ return zfld_; }
    LineSubSel		getSampling() const;
    Pos::GeomID		getGeomID() const	{ return gid_; }
    trcnr_steprg_type	getTrcNrRange() const;
    ZSampling		getZRange() const;

    void		setSampling(const LineSubSel&);
    void		displayStep(bool yn) override;

private:

    Pos::GeomID		gid_;
    uiSelNrRange*	nrfld_;
    uiSelZRange*	zfld_ = nullptr;

};


/*!\brief Selects 2D geometries with their ranges. */

mExpClass(uiIo) uiSelSublineSet : public uiSelRangeBase
{ mODTextTranslationClass(uiSelGeoms)
public:
			uiSelSublineSet(uiParent*,bool wstep=true,
					bool withz=true,
					const LineSubSelSet* =nullptr);
			~uiSelSublineSet();

   LineSubSelSet	getSampling() const;

   void			setSampling(const LineSubSelSet&);
   void			displayStep(bool yn) override;

   CNotifier<uiSelSublineSet,Pos::GeomID>	geomChanged;

private:

    uiGenInput*		linenmsfld_;
    ObjectSet<uiSelSubline>	linergsfld_;
    const bool		withstep_;
    const bool		withz_;

    void		initGrp(CallBacker*);
    void		lineChgCB(CallBacker*);
    void		adaptLineSelIfNeeded(const LineSubSelSet&);

    static void		getLineNames(const LineSubSelSet&,
				     BufferStringSet&);

};
