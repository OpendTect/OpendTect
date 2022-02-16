#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Dec 2014
________________________________________________________________________

-*/

#include "uiwellattribmod.h"

#include "uigroup.h"
#include "uiraytrace1d.h"
#include "uiseiswvltsel.h"
#include "uistring.h"

class EnumDef;
class SynthGenParams;
class uiGenInput;
class uiLabeledComboBox;
class uiLabeledListBox;
class uiSynthSeisSel;


/*!
\brief Interface for selecting all parameters to run a Seis::RaySynthGenerator
	for multiple synthetic types (Zero-offset, prestack, ...)
*/

mExpClass(uiWellAttrib) uiMultiSynthSeisSel : public uiGroup
{ mODTextTranslationClass(uiMultiSynthSeisSel);
public:

    mExpClass(uiWellAttrib) Setup : public uiSeisWaveletSel::Setup
    {
	public:
			Setup(const char* wvltseltxt="Wavelet")
			    : uiSeisWaveletSel::Setup(wvltseltxt)
			    , withzeroff_(true)
			    , withelasticstack_(false)
			    , withps_(true)
			{}

	mDefSetupMemb(bool,withzeroff)
	mDefSetupMemb(bool,withelasticstack)
	mDefSetupMemb(bool,withps)
	const uiRayTracer1D::Setup* rtsu_ = nullptr;
    };

				uiMultiSynthSeisSel(uiParent*,
						    const Setup& =Setup());
    virtual			~uiMultiSynthSeisSel();

    virtual uiRetVal		isOK() const;

    void			setType(const char*);
    void			setWavelet(const MultiID&);
    void			setWavelet(const char*);
    virtual bool		usePar(const IOPar&);

    const char*			getType() const;
    MultiID			getWaveletID() const;
    const char*			getWaveletName() const;
    virtual void		fillPar(IOPar&) const;

    Notifier<uiMultiSynthSeisSel> selectionChanged;
    Notifier<uiMultiSynthSeisSel> parsChanged;

protected:
				uiMultiSynthSeisSel(uiParent*,const Setup&,
						    bool withderived);

    void			initGrpCB(CallBacker*);
    void			selChgCB(CallBacker*);
    void			parsChangedCB(CallBacker*);

    virtual void		selChg(const char*);
    virtual void		initGrp();
    virtual void		doParsChanged(IOPar* prev=nullptr);
    uiGroup*			topGrp() const		{ return topgrp_; }
    uiLabeledComboBox*		typeCBFld() const	{ return typelblcbx_; }

private:

    const uiSynthSeisSel*	current() const;
    uiSynthSeisSel*		current();

    bool			useSynthSeisPar(const IOPar&);
    bool			useReflPars(const IOPar&);
    void			fillSynthSeisPar(IOPar&) const;
    void			fillReflPars(IOPar&) const;

    EnumDef&			typedef_;

    uiGroup*			topgrp_;
    uiLabeledComboBox*		typelblcbx_ = nullptr;
    uiSynthSeisSel*		zerooffsynthgrp_ = nullptr;
    uiSynthSeisSel*		elasticsynthgrp_ = nullptr;
    uiSynthSeisSel*		prestacksynthgrp_ = nullptr;
    uiSynthSeisSel*		previoussynthgrp_ = nullptr;
    ObjectSet<uiSynthSeisSel>	synthgrps_;

};


/*!
\brief Interface for selecting all parameters to run a Seis::RaySynthGenerator
	for multiple synthetic types (Zero-offset, prestack, ...), and
	interface for selecting synthetic datasets derived from raw types
*/

mExpClass(uiWellAttrib) uiFullSynthSeisSel : public uiMultiSynthSeisSel
{ mODTextTranslationClass(uiFullSynthSeisSel);
public:
				uiFullSynthSeisSel(uiParent*,
						    const Setup& =Setup());
				~uiFullSynthSeisSel();

    uiRetVal			isOK() const override;
    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

    void			manPSSynth(const char*,bool isnew);
    void			manInpSynth(const char*,bool isnew);
    void			getChosenInstantAttribs(BufferStringSet&) const;

    CNotifier<uiMultiSynthSeisSel,BufferString> nameChanged;

private:

    void			inputChangedCB(CallBacker*);
    void			nameChangedCB(CallBacker*);

    void			selChg(const char*) override;
    void			doParsChanged(IOPar* prev=nullptr) override;
    void			setOutputName(const char*);
    const char*			getOutputName() const;
    static void			doMan(uiComboBox*,const char*,bool isnew);

    uiLabeledComboBox*		psselfld_;
    uiLabeledComboBox*		inpselfld_;
    uiGenInput*			angleinpfld_;
    uiLabeledListBox*		instattribfld_;
    uiGenInput*			namefld_;

};
