#ifndef uistratsynthdisp_h
#define uistratsynthdisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uigroup.h"
#include "uiflatviewslicepos.h"
#include "stratsynth.h"

class TimeDepthModel;
class SeisTrcBuf;
class Wavelet;
class uiComboBox;
class uiFlatViewer;
class uiMultiFlatViewControl;
class uiPushButton;
class uiSynthGenDlg;
class uiSeisWaveletSel;
class uiSynthSlicePos;
class uiToolButton;
class uiToolButtonSetup;
namespace Strat { class LayerModel; }
namespace FlatView { class AuxData; }
namespace PreStackView { class uiSyntheticViewer2DMainWin; }


mExpClass(uiWellAttrib) uiStratSynthDisp : public uiGroup
{
public:

    			uiStratSynthDisp(uiParent*,const Strat::LayerModel&,
					 const Strat::LayerModel&);
    			~uiStratSynthDisp();

    const Strat::LayerModel& layerModel() const;	
    const char*		levelName() const;
    const MultiID&	waveletID() const;
    const Wavelet*	getWavelet() const;

    const ObjectSet<SyntheticData>& getSynthetics() const;
    SyntheticData*	getCurrentSyntheticData(bool wva=true) const;
    const SeisTrcBuf&	postStackTraces(const PropertyRef* pr=0) const;
    const PropertyRefSelection&	modelPropertyRefs() const;

    const ObjectSet<const TimeDepthModel>* d2TModels() const;

    void		setDispMrkrs(const char* lvlnm,const TypeSet<float>&,
	    			     Color,bool);
    void		setSelectedTrace(int);
    void		setDispEach(int);
    void		setZDataRange(const Interval<double>&,bool indpt);
    void		setDisplayZSkip(float zskip,bool withmodchg);

    const uiWorldRect&	curView(bool indepth) const;

    uiFlatViewer*	viewer()		{ return vwr_; }

    Notifier<uiStratSynthDisp>	wvltChanged;
    Notifier<uiStratSynthDisp>	zoomChanged;
    Notifier<uiStratSynthDisp>	viewChanged;
    Notifier<uiStratSynthDisp>	layerPropSelNeeded;
    Notifier<uiStratSynthDisp>	modSelChanged;
    Notifier<uiStratSynthDisp>	synthsChanged;

    void		addTool(const uiToolButtonSetup&);
    void		addViewerToControl(uiFlatViewer&);

    void		modelChanged();
    bool		haveUserScaleWavelet();
    void		displaySynthetic(const SyntheticData*);
    void		cleanSynthetics();
    float		centralTrcShift() const;
    void		setCurrentSynthetic(bool wva);
    void		setSnapLevelSensitive(bool);
    bool		prepareElasticModel();

    uiMultiFlatViewControl* control() 	{ return control_; }

    void		fillPar(IOPar&) const;
    void		fillPar(IOPar&,bool) const;
    bool		usePar(const IOPar&);

    void		setBrineFilled( bool yn ) { isbrinefilled_ = yn; }
    void		setAutoUpdate( bool yn ) { autoupdate_ = yn; }
	void		setUseEdited( bool yn )	  { useed_ = yn; }

protected:

    int			longestaimdl_;
    StratSynth*		stratsynth_;
    StratSynth*		edstratsynth_;
    const Strat::LayerModel& lm_;
    int			selectedtrace_;
    int			dispeach_;
    float		dispskipz_;
    bool		dispflattened_;
    bool		isbrinefilled_;
    bool		autoupdate_;
	bool		useed_;

    const ObjectSet<const TimeDepthModel>* d2tmodels_;
    SyntheticData* 	currentwvasynthetic_;
    SyntheticData* 	currentvdsynthetic_;

    uiMultiFlatViewControl* control_;
    FlatView::AuxData*	selectedtraceaux_;
    FlatView::AuxData*	levelaux_;

    uiGroup*		topgrp_;
    uiGroup*		datagrp_;
    uiGroup*		prestackgrp_;
    uiSeisWaveletSel*	wvltfld_;
    uiFlatViewer*	vwr_;
    uiPushButton*	scalebut_;
    uiToolButton*	lasttool_;
    uiToolButton*	prestackbut_;
    uiToolButton*	addeditbut_;
    uiComboBox*		wvadatalist_;
    uiComboBox*		vddatalist_;
    uiComboBox*		levelsnapselfld_;
    uiSynthGenDlg*	synthgendlg_;
    uiSynthSlicePos*	offsetposfld_;
    PtrMan<TaskRunner>	taskrunner_;
    PreStackView::uiSyntheticViewer2DMainWin*	prestackwin_;

    void		setCurrentWavelet();
    void		fillPar(IOPar&,const StratSynth*) const;
    void		doModelChange();
    const SeisTrcBuf&	curTrcBuf() const;
    void		updateFields();
    void		updateSynthetic(const char* nm,bool wva);
    void		updateSyntheticList(bool wva);

    void		drawLevel();
    void		displayFRText();
    void		displayPreStackSynthetic(const SyntheticData*);
    void		displayPostStackSynthetic(const SyntheticData*,
	    					  bool wva=true);

    void		addEditSynth(CallBacker*);
    void		wvDataSetSel(CallBacker*);
    void		vdDataSetSel(CallBacker*);
    void		levelSnapChanged(CallBacker*);
    void		layerPropsPush(CallBacker*);
    void		offsetChged(CallBacker*);
    void		scalePush(CallBacker*);
    void 		genNewSynthetic(CallBacker*);
    void		viewPreStackPush(CallBacker*);
    void		wvltChg(CallBacker*);
    void		zoomChg(CallBacker*);
    void		viewChg(CallBacker*);
    void		syntheticRemoved(CallBacker*);
    void		syntheticChanged(CallBacker*);
    void		selPreStackDataCB(CallBacker*);
};


mExpClass(uiWellAttrib) uiSynthSlicePos : public uiGroup
{
public:
    			uiSynthSlicePos(uiParent*,const char* lbltxt);

    Notifier<uiSynthSlicePos>	positionChg;
    void		setLimitSampling(StepInterval<float>);
    int			getValue() const;
    void		setValue(int) const;

protected:
    uiLabel* 		label_;
    uiSpinBox*		sliceposbox_;
    uiSpinBox*		slicestepbox_;
    uiToolButton*	prevbut_;
    uiToolButton*	nextbut_;

    void		slicePosChg( CallBacker* );
    void		prevCB(CallBacker*);
    void		nextCB(CallBacker*);

    StepInterval<float>	limitsampling_;
};

#endif

