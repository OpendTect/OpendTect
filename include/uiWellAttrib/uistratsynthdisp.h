#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uigroup.h"
#include "uiflatviewslicepos.h"
#include "uistring.h"

class PropertyRef;
class PropertyRefSelection;
class SeisTrcBuf;
class StratSynth;
class SyntheticData;
class SynthFVSpecificDispPars;
class TaskRunner;
class TimeDepthModel;
class Wavelet;

class uiButton;
class uiComboBox;
class uiFlatViewer;
class uiMultiFlatViewControl;
class uiPushButton;
class uiSynthGenDlg;
class uiSeisWaveletSel;
class uiStratLayerModel;
class uiSynthSlicePos;
class uiTextItem;
class uiToolButton;
class uiToolButtonSetup;
namespace Strat { class LayerModel; class LayerModelProvider; }
namespace FlatView { class AuxData; class Appearance; }
namespace PreStackView { class uiSyntheticViewer2DMainWin; }


mExpClass(uiWellAttrib) uiStratSynthDisp : public uiGroup
{ mODTextTranslationClass(uiStratSynthDisp);
public:

			uiStratSynthDisp(uiParent*,
					 const Strat::LayerModelProvider&);
			~uiStratSynthDisp();

    const Strat::LayerModel& layerModel() const;
    const char*		levelName() const;
    const MultiID&	waveletID() const;
    const Wavelet*	getWavelet() const;
    inline const StratSynth& curSS() const
			{ return *(!useed_ ? stratsynth_ : edstratsynth_); }
    inline StratSynth&	curSS()
			{ return *(!useed_ ? stratsynth_ : edstratsynth_); }
    inline const StratSynth& altSS() const
			{ return *(useed_ ? stratsynth_ : edstratsynth_); }
    const StratSynth&	normalSS() const	{ return *stratsynth_; }
    const StratSynth&	editSS() const		{ return *edstratsynth_; }

    const ObjectSet<const SyntheticData>& getSynthetics() const;

    RefMan<SyntheticData> getSyntheticData(const char* nm);
    ConstRefMan<SyntheticData> getSyntheticData(const char* nm) const;
    ConstRefMan<SyntheticData> getCurrentSyntheticData(bool wva=true) const;
    const PropertyRefSelection& modelPropertyRefs() const;

    const SynthFVSpecificDispPars* dispPars(const char* synthnm) const;
    SynthFVSpecificDispPars* dispPars(const char* synthnm);
    const ObjectSet<const TimeDepthModel>* d2TModels() const;

    void		setFlattened(bool flattened,bool trigger=true);
    void		setDispMrkrs(const char* lvlnm,const TypeSet<float>&,
				     OD::Color);
    void		setSelectedTrace(int);
    void		setDispEach(int);
    void		setZDataRange(const Interval<double>&,bool indpt);
    void		setDisplayZSkip(float zskip,bool withmodchg);
    void		displayFRText(bool yn,bool isbrine);

    const uiWorldRect&	curView(bool indepth) const;
    void		setZoomView(const uiWorldRect&);

    uiFlatViewer*	viewer()		{ return vwr_; }

    Notifier<uiStratSynthDisp>	wvltChanged;
    Notifier<uiStratSynthDisp>	viewChanged;
    Notifier<uiStratSynthDisp>	layerPropSelNeeded;
    Notifier<uiStratSynthDisp>	modSelChanged;
    Notifier<uiStratSynthDisp>	synthsChanged;
    Notifier<uiStratSynthDisp>	dispParsChanged;

    void		addTool(const uiToolButtonSetup&);
    void		addViewerToControl(uiFlatViewer&);

    mDeprecatedDef void modelChanged();
    bool		haveUserScaleWavelet();
    void		displaySynthetic(const SyntheticData*);
    void		reDisplayPostStackSynthetic(bool wva=true);
    void		cleanSynthetics();
    float		centralTrcShift() const;
    void		setCurrentSynthetic(bool wva);
    void		setSnapLevelSensitive(bool);
    bool		prepareElasticModel();

    uiMultiFlatViewControl* control()	{ return control_; }

    void		fillPar(IOPar&) const;
    void		fillPar(IOPar&,bool) const;
    bool		usePar(const IOPar&);
    void		makeInfoMsg(BufferString& msg,IOPar&);

    void		showFRResults();
    mDeprecatedDef void setBrineFilled( bool yn ) { isbrinefilled_ = yn; }
    void		setAutoUpdate( bool yn )  { autoupdate_ = yn; }
    void		setForceUpdate( bool yn ) { forceupdate_ = yn; }
    bool		doForceUpdate() const	  { return forceupdate_; }
    void		setUseEdited( bool yn )	  { useed_ = yn; }
    bool		isEditUsed() const	  { return useed_; }
    void		setDiffData();
    void		resetRelativeViewRect();
    void		updateRelativeViewRect();
    void		setRelativeViewRect(const uiWorldRect& relwr);
    const uiWorldRect&	getRelativeViewRect() const	{ return relzoomwr_; }
    void		setSavedViewRect();

    uiGroup*		getDisplayClone(uiParent*) const;

protected:

    int			longestaimdl_ = 0;
    StratSynth*		stratsynth_;
    StratSynth*		edstratsynth_;
    const Strat::LayerModelProvider& lmp_;
    uiWorldRect		relzoomwr_;
    mutable uiWorldRect	savedzoomwr_;
    int			selectedtrace_ = -1;
    int			dispeach_ = 1;
    float		dispskipz_ = 0.f;
    bool		dispflattened_ = false;
    /*mDeprecated*/ bool	isbrinefilled_;
    bool		autoupdate_ = true;
    bool		forceupdate_ = false;
    bool		useed_ = false;

    const ObjectSet<const TimeDepthModel>* d2tmodels_ = nullptr;
    WeakPtr<SyntheticData> currentwvasynthetic_;
    WeakPtr<SyntheticData> currentvdsynthetic_;

    uiMultiFlatViewControl* control_;
    FlatView::AuxData*	selectedtraceaux_ = nullptr;
    FlatView::AuxData*	levelaux_ = nullptr;

    uiGroup*		topgrp_;
    uiGroup*		datagrp_;
    uiGroup*		prestackgrp_;
    uiSeisWaveletSel*	wvltfld_;
    uiFlatViewer*	vwr_;
    uiPushButton*	scalebut_;
    uiButton*		lasttool_ = nullptr;
    uiToolButton*	prestackbut_;
    uiComboBox*		wvadatalist_;
    uiComboBox*		vddatalist_;
    uiComboBox*		levelsnapselfld_;
    uiSynthGenDlg*	synthgendlg_ = nullptr;
    uiSynthSlicePos*	offsetposfld_;
    uiTextItem*     frtxtitm_ = nullptr;
    PtrMan<TaskRunner>	taskrunner_;
    PreStackView::uiSyntheticViewer2DMainWin*	prestackwin_ = nullptr;

    void		showInfoMsg(bool foralt);
    void		handleFlattenChange();
    void		setCurrentWavelet();
    void		fillPar(IOPar&,const StratSynth*) const;
    void		doModelChange();
    const SeisTrcBuf&	curTrcBuf() const;
    int			getOffsetIdx(const SyntheticData&) const;
    void		getCurD2TModel(const SyntheticData&,
				    ObjectSet<const TimeDepthModel>&,
				    int offsidx) const;
    void		reSampleTraces(const SyntheticData&,SeisTrcBuf&) const;
    void		updateFields();
    void		updateSynthetic(const char* nm,bool wva);
    void		updateSyntheticList(bool wva);
    void		setCurSynthetic(const SyntheticData*,bool wva);
    void		copySyntheticDispPars();
    void		setDefaultAppearance(FlatView::Appearance&);
    inline StratSynth&	altSS()
			{ return *(useed_ ? stratsynth_ : edstratsynth_); }

    void		drawLevel();
    mDeprecatedDef void displayFRText();
    void		displayPreStackSynthetic(const SyntheticData*);
    void		displayPostStackSynthetic(const SyntheticData*,
						  bool wva=true);
    void		updateTextPosCB(CallBacker*);

    void		setPreStackMapper();
    void		setAbsoluteViewRect(const uiWorldRect& abswr);
    void		getAbsoluteViewRect(uiWorldRect& abswr) const;

    void		addEditSynth(CallBacker*);
    void		exportSynth(CallBacker*);
    void		wvDataSetSel(CallBacker*);
    void		vdDataSetSel(CallBacker*);
    void		levelSnapChanged(CallBacker*);
    void		layerPropsPush(CallBacker*);
    void		offsetChged(CallBacker*);
    void		scalePush(CallBacker*);
    void		viewPreStackPush(CallBacker*);
    void		wvltChg(CallBacker*);
    void		viewChg(CallBacker*);
    void		parsChangedCB(CallBacker*);
    void		syntheticAdded(CallBacker*);
    void		syntheticChanged(CallBacker*);
    void		syntheticRemoved(CallBacker*);
    void		syntheticDisabled(CallBacker*);
    void		selPreStackDataCB(CallBacker*);
    void		preStackWinClosedCB(CallBacker*);
    void		newModelsCB(CallBacker*);
    void		uiTaskRunDeletedCB(CallBacker*);

public:

			//6.2 only: for attaching a notifier
    void		set(uiStratLayerModel&);

};


mExpClass(uiWellAttrib) uiSynthSlicePos : public uiGroup
{ mODTextTranslationClass(uiSynthSlicePos);
public:
			uiSynthSlicePos(uiParent*,const uiString& lbltxt);
			~uiSynthSlicePos();

    Notifier<uiSynthSlicePos>	positionChg;
    void		setLimitSampling(StepInterval<float>);
    int			getValue() const;
    void		setValue(int) const;

protected:
    uiLabel*		label_;
    uiSpinBox*		sliceposbox_;
    uiSpinBox*		slicestepbox_;
    uiToolButton*	prevbut_;
    uiToolButton*	nextbut_;

    void		slicePosChg( CallBacker* );
    void		prevCB(CallBacker*);
    void		nextCB(CallBacker*);

    StepInterval<float>	limitsampling_;
};

