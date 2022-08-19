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

class uiButton;
class uiGenInput;
namespace Pos { class Provider; }
class uiPosProvGroup;
class TrcKeyZSampling;

/*! \brief lets user choose a way to provide positions */

mExpClass(uiIo) uiPosProvider : public uiGroup
{ mODTextTranslationClass(uiPosProvider)
public:

    struct Setup : public uiPosProvGroup::Setup
    {
	enum ChoiceType	{ All, OnlySeisTypes, OnlyRanges, RangewithPolygon,
       			  VolumeTypes };

			Setup( bool is_2d, bool with_step, bool with_z )
			    : uiPosProvGroup::Setup(is_2d,with_step,with_z)
			    , seltxt_(uiStrings::sPosition(2))
			    , allownone_(false)
			    , useworkarea_(true)
			    , choicetype_(OnlyRanges)	{}
	virtual		~Setup()			{}

	mDefSetupMemb(uiString,seltxt)
	mDefSetupMemb(ChoiceType,choicetype)
	mDefSetupMemb(bool,allownone)
	mDefSetupMemb(bool,useworkarea)
    };

			uiPosProvider(uiParent*,const Setup&);
			~uiPosProvider();

    void		usePar(const IOPar&);
    bool		fillPar(IOPar&) const;
    void		setExtractionDefaults();

    void		setSampling(const TrcKeyZSampling&);
    void		getSampling(TrcKeyZSampling&,const IOPar* =0) const;

    Notifier<uiPosProvider>	posProvGroupChanged;
    bool		hasRandomSampling() const;

    Pos::Provider*	createProvider() const;

    bool		is2D() const		{ return setup_.is2d_; }
    bool		isAll() const		{ return !curGrp(); }

protected:

    uiGenInput*			selfld_;
    uiButton*			fullsurvbut_;
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
    void		fillPar( IOPar& iop ) const	{ iop.merge(iop_); }

    Pos::Provider*	curProvider()			{ return prov_; }
    const Pos::Provider* curProvider() const		{ return prov_; }

    const TrcKeyZSampling&	envelope() const;
    void		setInput(const TrcKeyZSampling&,bool chgtype=true);
    void		setInput(const TrcKeyZSampling& initcs,
				 const TrcKeyZSampling& ioparcs);
    void		setInputLimit(const TrcKeyZSampling&);
    const TrcKeyZSampling&	inputLimit() const	{ return setup_.tkzs_; }

    bool		isAll() const;
    void		setToAll();

protected:

    Setup		setup_;
    IOPar		iop_;
    Pos::Provider*	prov_;
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
    void		getSampling(TrcKeyZSampling&,const IOPar* =0) const;

protected:
    bool		acceptOK(CallBacker*) override;

    uiPosProvider*	selfld_;
};
