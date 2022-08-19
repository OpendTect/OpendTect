#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"

#include "enums.h"
#include "factory.h"
#include "synthseis.h"
#include "uigroup.h"
#include "uiraytrace1d.h"
#include "uiseiswvltsel.h"
#include "uistring.h"

class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiPushButton;
class uiSynthSeisAdvancedDlg;
class uiSynthSeisAdvancedGrp;


/*!
\brief Interface for selecting all parameters to run a Seis::RaySynthGenerator
*/

mExpClass(uiSeis) uiSynthSeis : public uiGroup
{ mODTextTranslationClass(uiSynthSeis);
public:

    mExpClass(uiSeis) Setup
    {
	public:
			Setup(bool withnmo,const uiSeisWaveletSel::Setup&
				=uiSeisWaveletSel::Setup());
			~Setup();

	mDefSetupMemb(uiSeisWaveletSel::Setup,wvltsu)
	mDefSetupMemb(bool,withnmo);
	mDefSetupMemb(bool,withadvanced);
    };

    mDefineFactory2ParamInClass(uiSynthSeis,uiParent*,const Setup&,factory);

			~uiSynthSeis();

    void		setWavelet(const MultiID&);
    void		setWavelet(const char*);
    void		ensureHasWavelet(const MultiID&);
    virtual bool	usePar(const IOPar&);

    virtual uiRetVal	isOK() const;
    MultiID		getWaveletID() const;
    const char*		getWaveletName() const;
    virtual void	fillPar(IOPar&) const;

    Notifier<uiSynthSeis> parsChanged;

protected:
			uiSynthSeis(uiParent*,const Setup&);

    bool		withNMO() const		{ return withnmo_; }
    uiSeisWaveletSel*	wvltFld() const		{ return wvltfld_; }
    uiObject*		lastFld() const;

    virtual void	initGrp();
    void		ensureHasAdvancedButton();
    void		setAdvancedGroup(uiSynthSeisAdvancedGrp*);
    void		parsChangedCB(CallBacker*);

private:

    void		initGrpCB(CallBacker*);
    void		getAdvancedPush(CallBacker*);

    bool		withnmo_;
    IOPar*		lastiop_ = nullptr;
    uiSeisWaveletSel*	wvltfld_;
    uiPushButton*	advbut_ = nullptr;
    uiSynthSeisAdvancedGrp* advgrp_ = nullptr;
    uiSynthSeisAdvancedDlg* advdlg_ = nullptr;

public:

    virtual uiSynthSeisAdvancedGrp* getAvancedGrp(uiParent*);

};


mExpClass(uiSeis) uiBaseSynthSeis : public uiSynthSeis
{ mODTextTranslationClass(uiBaseSynthSeis);
public:
    mDefaultFactoryInstanciationBase(
			Seis::SynthGeneratorBasic::sFactoryKeyword(),
			Seis::SynthGeneratorBasic::sFactoryDisplayName());

			uiBaseSynthSeis(uiParent*,
					const uiSynthSeis::Setup&);

    static uiSynthSeis* create( uiParent* p, const uiSynthSeis::Setup& su )
			    { return new uiBaseSynthSeis( p, su ); }
};


mExpClass(uiSeis) uiSynthSeisSel : public uiGroup
{ mODTextTranslationClass(uiSynthSeisSel);
public:

			uiSynthSeisSel(uiParent*,const uiSynthSeis::Setup&);
			uiSynthSeisSel(uiParent*,const uiSynthSeis::Setup&,
				       const uiRayTracer1D::Setup&);
			~uiSynthSeisSel();

    uiRetVal		isOK() const;

    void		setWavelet(const MultiID&);
    void		setWavelet(const char*);
    void		ensureHasWavelet(const MultiID&);
    void		usePar(const IOPar&);
    void		useSynthSeisPar(const IOPar&);
    void		useReflPars(const IOPar&);

    MultiID		getWaveletID() const;
    const char*		getWaveletName() const;
    void		fillPar(IOPar&) const;
    void		fillSynthSeisPar(IOPar&) const;
    void		fillReflPars(IOPar&) const;

    Notifier<uiSynthSeisSel> parsChanged;

private:

    uiGroup*		uiseisfldsgrp_;
    uiComboBox*		synthseisselfld_ = nullptr;
    ObjectSet<uiSynthSeis> grps_;
    uiRayTracerSel*	rtsel_ = nullptr;

    void		setDefault();
    bool		setCurrentType(const char*);

    bool		withRefl() const;
    const uiSynthSeis*	current() const;
    uiSynthSeis*	current();

    void		initGrpCB(CallBacker*);
    void		selSynthSeisCB(CallBacker*);
    void		parsChangedCB(CallBacker*);
};


/*!
\brief Group for the advanced synthetic generation parameters interface
*/

mExpClass(uiSeis) uiSynthSeisAdvancedGrp : public uiGroup
{ mODTextTranslationClass(uiSynthSeisAdvancedGrp);
public:
    enum ConvDomain		{ Freq, TWT };
				mDeclareEnumUtils(ConvDomain)

				uiSynthSeisAdvancedGrp(uiParent*,bool withnmo);
				~uiSynthSeisAdvancedGrp();

    virtual uiRetVal		isOK() const;
    virtual bool		usePar(const IOPar&);
				//<! return false if not changed
    virtual void		fillPar(IOPar&) const;

    Notifier<uiSynthSeisAdvancedGrp> parsChanged;

protected:

    void			parsChangedCB(CallBacker*);
    virtual void		initGrp();

    uiObject*			lastFld() const;

private:

    uiGenInput*			convdomainfld_;
    uiGenInput*			nmofld_ = nullptr;
    uiGenInput*			stretchmutelimitfld_ = nullptr;
    uiGenInput*			mutelenfld_ = nullptr;

    void			createNMOGrp();

    void			initGrpCB(CallBacker*);
    void			nmoSelCB(CallBacker*);

    void			setStretchMutePerc(double);
    void			setMuteLengthMs(double);

    bool			withNMO() const		{ return nmofld_; }
    bool			wantNMOCorr() const;
    double			getStrechtMutePerc() const;
    double			getMuteLengthMs() const;

};
