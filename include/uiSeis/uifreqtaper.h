#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uibutton.h"
#include "uidialog.h"
#include "uifuncdispbase.h"
#include "uiwindowfunctionsel.h"

#include "survinfo.h"
#include "arrayndalgo.h"
#include <arrayndimpl.h>
#include "uistring.h"

class uiFreqTaperGrp;
class uiFuncTaperDisp;
class uiGenInput;
class uiSliceSelDlg;
class uiSlider;

class ArrayNDWindow;
class TrcKeyZSampling;


mStruct(uiSeis) FreqTaperSetup
{
			FreqTaperSetup();
			FreqTaperSetup(const FreqTaperSetup&);
			~FreqTaperSetup();

    const char*		seisnm_			= nullptr;
    const char*		attrnm_			= nullptr; //2D
    bool		hasmin_			= false;
    bool		hasmax_			= true;
    Interval<float>	minfreqrg_;
    Interval<float>	maxfreqrg_;
    bool		allfreqssetable_	= false;
    MultiID		multiid_		= MultiID::udf();
};


mStruct(uiSeis) TaperData
{
			TaperData();
			~TaperData();

    Interval<float>	rg_;
    Interval<float>	refrg_;

    ArrayNDWindow*	window_			= nullptr;
    int			winsz_			= 0;
    float		paramval_		= 1;
    float		slope_			= mUdf(float);
};


mExpClass(uiSeis) uiFuncTaperDisp : public uiGroup
{ mODTextTranslationClass(uiFuncTaperDisp);
public:

    struct Setup : public uiFuncDispBase::Setup
    {
			Setup()
			    : is2sided_(false)
			    , datasz_((int)(0.5/SI().zStep() *
				    (SI().zDomain().isTime() ? 1.0f : 1000.0f)))
			    , logscale_(false)
			    {
				xaxcaption_ = uiStrings::phrJoinStrings(
					    SI().zDomain().isTime() ?
						    uiStrings::sFrequency() :
						    uiStrings::sWaveNumber(),
					    SI().zDomain().isTime() ?
						    tr("(Hz)") :
					    SI().depthsInFeet() ?
						    tr("(/kft)") :
						    tr("(/km)") );
				yaxcaption_ = tr("Amplitude");
				noxgridline_ = true;
				noygridline_ = true;
				ylinestyle_.color_.set( 200, 0, 0 );
				y2linestyle_.color_.set( 0, 0, 220 );
			    }
			~Setup()
			{}

	mDefSetupMemb(int,datasz);
	mDefSetupMemb(uiString,xaxcaption);
	mDefSetupMemb(uiString,yaxcaption);
	mDefSetupMemb(Interval<float>,leftrg)
	mDefSetupMemb(Interval<float>,rightrg)
	mDefSetupMemb(bool,is2sided);
	mDefSetupMemb(bool,logscale);
    };

			uiFuncTaperDisp(uiParent*,const Setup&);
			~uiFuncTaperDisp();

    void		setWindows(float,float rightvar=0);
    void		setFunction(Array1DImpl<float>&,Interval<float>);

    ArrayNDWindow*	window() const		{ return window_; }

    float*		getWinValues() const
			{ return window_ ? window_->getValues() : 0; }
    float*		getFuncValues() const
			{ return funcvals_ ? funcvals_->getData() : 0; }

    void		adaptFreqRangesToDataSize(bool,bool);

    void		updateDisplay();

    TaperData&		leftTaperData()		{ return leftd_; }
    TaperData&		rightTaperData()	{ return rightd_; }

    int			dataSize() const	{ return datasz_; }
    uiFuncDispBase&	disp()			{ return *disp_; }

    Notifier<uiFuncTaperDisp> taperChanged;

protected:

    void		taperChangedCB(CallBacker*);

    uiFuncDispBase*	disp_;
    TaperData		leftd_;
    TaperData		rightd_;

    ArrayNDWindow*	window_			= nullptr;

    Array1DImpl<float>* funcvals_		= nullptr;
    Array1DImpl<float>* orgfuncvals_		= nullptr;
    Interval<float>	funcdisprg_;

    bool		is2sided_;
    bool		logscale_;
    int			datasz_;
    int			orgdatasz_;
};



mExpClass(uiSeis) uiFreqTaperGrp : public uiGroup
{ mODTextTranslationClass(uiFreqTaperGrp);

public:
			uiFreqTaperGrp(uiParent*,
				       const FreqTaperSetup&,
				       uiFuncTaperDisp*);
			~uiFreqTaperGrp();

    void		setFreqRange(Interval<float>);
    Interval<float>	getFreqRange() const;

    void		taperChanged();

protected :

    TaperData		td1_;
    TaperData		td2_;

    uiGenInput*		varinpfld_;
    uiGenInput*		freqinpfld_		= nullptr;
    uiGenInput*		inffreqfld_;
    uiGenInput*		supfreqfld_;
    uiSlider*		sliderfld_;
    uiFuncTaperDisp*    drawer_;

    bool		hasmin_;
    bool		hasmax_;
    bool		isminactive_;
    int			datasz_;
    bool		allfreqssetable_;

    void		setSlopeFromFreq();
    void		setPercentsFromFreq();
    void		setFreqFromSlope(float);

    void		freqChangedCB(CallBacker*);
    void		freqInpChgdCB(CallBacker*);
    void		putToScreenCB(CallBacker*);
    void		sliderChangedCB(CallBacker*);
    void		slopeChangedCB(CallBacker*);
    void		taperChangedCB(CallBacker*);
};



mExpClass(uiSeis) uiFreqTaperDlg : public uiDialog
{ mODTextTranslationClass(uiFreqTaperDlg);
public:

			uiFreqTaperDlg(uiParent*,const FreqTaperSetup&);
			~uiFreqTaperDlg();

    Interval<float>	getFreqRange() const
			{ return tapergrp_->getFreqRange(); }

protected:

    uiFreqTaperGrp*	tapergrp_;
    uiFuncTaperDisp*    drawer_;
    Array1DImpl<float>* funcvals_		= nullptr;

    const char*		seisnm_;
    const char*		attrnm_;
    uiPushButton*	previewfld_;
    uiSliceSelDlg*	posdlg_			= nullptr;
    TrcKeyZSampling*	tkzs_;
    MultiID		seisid_;

    void		postFinalizeCB(CallBacker*);
    void		previewPushedCB(CallBacker*);
};



mExpClass(uiSeis) uiFreqTaperSel : public uiWindowFunctionSel
{ mODTextTranslationClass(uiFreqTaperSel);
public:
			uiFreqTaperSel(uiParent*,const Setup&,
						const FreqTaperSetup&);
			~uiFreqTaperSel();

    Interval<float>	freqValues() const;

    void		setMultiID( const MultiID& multiid )
			{ freqsetup_.multiid_ = multiid; }
    void		setIsMinMaxFreq(bool,bool);
    void		setInputFreqValue(float,int);
    void		setRefFreqs(Interval<float>);

protected :

    uiFreqTaperDlg*	freqtaperdlg_		= nullptr;
    FreqTaperSetup	freqsetup_;

    void		winfuncseldlgCB(CallBacker*) override;
    void		windowClosedCB(CallBacker*);
    void		setSelFreqsCB(CallBacker*);
};
