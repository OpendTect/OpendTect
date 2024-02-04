#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"

#include "uicompoundparsel.h"
#include "uidialog.h"
#include "uiposprovgroup.h"
#include "iopar.h"

class TrcKeyZSampling;
class uiButton;
class uiGenInput;
class uiPosProvGroup;
namespace Pos { class Provider; }
namespace ZDomain { class Info; }

/*! \brief lets user choose a way to provide positions */

mExpClass(uiIo) uiPosProvider : public uiGroup
{ mODTextTranslationClass(uiPosProvider)
public:

    mExpClass(uiIo) Setup : public uiPosProvGroup::Setup
    {
    public:
	enum ChoiceType	{ All, OnlySeisTypes, OnlyRanges, RangewithPolygon,
			  VolumeTypes };

			Setup(bool is_2d,bool with_step,bool with_z);
	virtual		~Setup();

	mDefSetupMemb(uiString,seltxt)
	mDefSetupMemb(ChoiceType,choicetype)	// OnlyRanges
	mDefSetupMemb(bool,allownone)		// false
	mDefSetupMemb(bool,useworkarea)		// true
    };

			uiPosProvider(uiParent*,const Setup&);
			~uiPosProvider();

    void		usePar(const IOPar&);
    bool		fillPar(IOPar&) const;
    void		setExtractionDefaults();

    void		setSampling(const TrcKeyZSampling&);
    void		getSampling(TrcKeyZSampling&,
				    const IOPar* =nullptr) const;

    Notifier<uiPosProvider> posProvGroupChanged;
    bool		hasRandomSampling() const;

    Pos::Provider*	createProvider() const;

    bool		is2D() const		{ return setup_.is2d_; }
    bool		isAll() const		{ return !curGrp(); }

protected:

    uiGenInput*			selfld_ = nullptr;
    uiButton*			fullsurvbut_ = nullptr;
    uiButton*			openbut_;
    uiButton*			savebut_;
    ObjectSet<uiPosProvGroup>	grps_;
    Setup			setup_;

    void			selChg(CallBacker*);
    void			fullSurvPush(CallBacker*);
    void			openCB(CallBacker*);
    void			saveCB(CallBacker*);
    uiPosProvGroup*		curGrp() const;
};


/*!\brief CompoundParSel to capture a user's Pos::Provider wishes */

mExpClass(uiIo) uiPosProvSel : public uiCompoundParSel
{
public:

    typedef uiPosProvider::Setup Setup;

			uiPosProvSel(uiParent*,const Setup&);
			~uiPosProvSel();

    void		usePar(const IOPar&);
    void		fillPar( IOPar& iop ) const;

    Pos::Provider*	curProvider()			{ return prov_; }
    const Pos::Provider* curProvider() const		{ return prov_; }

    const TrcKeyZSampling&	envelope() const;
    void		setInput(const TrcKeyZSampling&,bool chgtype=true);
    void		setInput(const TrcKeyZSampling& initcs,
				 const TrcKeyZSampling& ioparcs);
    void		setInputLimit(const TrcKeyZSampling&);
    const TrcKeyZSampling&	inputLimit() const	{ return setup_.tkzs_; }
    const ZDomain::Info* zDomain() const;

    bool		isAll() const;
    void		setToAll();

protected:

    Setup		setup_;
    IOPar		iop_;
    Pos::Provider*	prov_ = nullptr;
    TrcKeyZSampling&	tkzs_;

    void		doDlg(CallBacker*);
    BufferString	getSummary() const override;
    void		setCSToAll() const;
    void		setProvFromCS();
    void		mkNewProv(bool updsumm=true);
};


/*!\brief Dialog to capture a user's Pos::Provider wishes */

mExpClass(uiIo) uiPosProvDlg : public uiDialog
{ mODTextTranslationClass(uiPosProvDlg)
public:
    typedef uiPosProvider::Setup Setup;

			uiPosProvDlg(uiParent*,const Setup&,const uiString&);
			~uiPosProvDlg();

    void		setSampling(const TrcKeyZSampling&);
    void		getSampling(TrcKeyZSampling&,
				    const IOPar* =nullptr) const;

protected:
    bool		acceptOK(CallBacker*) override;

    uiPosProvider*	selfld_;
};
