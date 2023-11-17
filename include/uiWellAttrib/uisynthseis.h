#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellattribmod.h"

#include "uigroup.h"
#include "uiraytrace1d.h"
#include "uirefltrace1d.h"
#include "uiseiswvltsel.h"
#include "uistring.h"

class EnumDef;
class SynthGenParams;
class uiFreqFilterSelFreq;
class uiGenInput;
class uiLabeledComboBox;
class uiLabeledListBox;
class uiLabeledSpinBox;
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
				Setup(const char* wvltseltxt="Wavelet");
	virtual			~Setup();

	mDefSetupMemb(bool,withzeroff)		// def: true
	mDefSetupMemb(bool,withps)		// def: true

	Setup&		withelasticstack(bool yn);
	Setup&		withelasticgather(bool yn);

	bool		withElasticStack() const;
	bool		withElasticGather() const;

	const uiReflCalc1D::Setup* reflsu_ = nullptr;
	const uiRayTracer1D::Setup* rtsu_ = nullptr;

    private:

	static bool	canDoElastic();

	bool withelasticstack_;
	bool withelasticgather_;
    };

				uiMultiSynthSeisSel(uiParent*,
						    const Setup& =Setup());
    virtual			~uiMultiSynthSeisSel();

    virtual uiRetVal		isOK() const;

    void			setType(const char*);
    void			setWavelet(const MultiID&);
    void			setWavelet(const char*);
    void			ensureHasWavelet(const MultiID&);
    virtual bool		setFrom(const SynthGenParams&);
    virtual bool		usePar(const IOPar&);

    const char*			getType() const;
    MultiID			getWaveletID() const;
    const char*			getWaveletName() const;
    bool			getGenParams(SynthGenParams&) const;
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
    uiSynthSeisSel*		elasticstacksynthgrp_ = nullptr;
    uiSynthSeisSel*		elasticgathersynthgrp_ = nullptr;
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
    bool			setFrom(const SynthGenParams&) override;

    void			manPSSynth(const BufferStringSet&);
    void			manInpSynth(const BufferStringSet&);
    void			getChosenInstantAttribs(BufferStringSet&) const;

    CNotifier<uiMultiSynthSeisSel,BufferString> nameChanged;

private:

    void			inputChangedCB(CallBacker*);
    void			nameChangedCB(CallBacker*);
    void			filterChgCB(CallBacker*);

    void			selChg(const char*) override;
    void			doParsChanged(IOPar* prev=nullptr) override;
    void			setOutputName(const char*);
    const char*			getOutputName() const;
    static void			doMan(uiComboBox*,const BufferStringSet&);

    uiLabeledComboBox*		psselfld_;
    uiLabeledComboBox*		inpselfld_;
    uiGenInput*			angleinpfld_;
    uiLabeledListBox*		instattribfld_;
    uiGenInput*			namefld_;
    uiGenInput*			filtertypefld_();
    uiFreqFilterSelFreq*	freqfld_();
    uiLabeledSpinBox*		smoothwindowfld_();
    uiGenInput*			filtertypefld_() const;
    uiFreqFilterSelFreq*	freqfld_() const;
    uiLabeledSpinBox*		smoothwindowfld_() const;
};
