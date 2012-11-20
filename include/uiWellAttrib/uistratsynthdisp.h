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
#include "uimainwin.h"
#include "uidialog.h"
#include "uiflatviewslicepos.h"
#include "stratsynth.h"
#include "valseriesevent.h"

class FlatDataPack;
class TimeDepthModel;
class SeisTrcBuf;
class Wavelet;
class RayParam;
class uiComboBox;
class uiGenInput;
class uiCheckBox;
class uiFlatViewer;
class uiRayTracerSel;
class uiListBox;
class uiLabeledComboBox;
class uiFlatViewMainWin;
class uiMultiFlatViewControl;
class uiOffsetSlicePos;
class uiPushButton;
class uiSynthGenDlg;
class uiSeisWaveletSel;
class uiStackGrp;
class uiSynthSlicePos;
class uiToolButton;
class uiToolButtonSetup;
namespace Strat { class LayerModel; }
namespace FlatView { class AuxData; }


mClass(uiWellAttrib) uiStratSynthDisp : public uiGroup
{
public:

    			uiStratSynthDisp(uiParent*,const Strat::LayerModel&);
    			~uiStratSynthDisp();

    const Strat::LayerModel& layerModel() const;	
    const char*		levelName() const;
    const MultiID&	waveletID() const;
    const Wavelet*	getWavelet() const;

    const ObjectSet<SyntheticData>& getSynthetics() const;
    void		genSyntheticsFor(const Strat::LayerModel&,SeisTrcBuf&);
    SyntheticData*	getCurrentSyntheticData() const;
    const SeisTrcBuf&	postStackTraces(const PropertyRef* pr=0) const;
    const PropertyRefSelection&	modelPropertyRefs() const;

    const ObjectSet<const TimeDepthModel>* d2TModels() const;

    void		setDispMrkrs(const char* lvlnm,const TypeSet<float>&,
	    			     Color,bool);
    void		setSelectedTrace(int);
    void		setDispEach(int);
    void		setZDataRange(const Interval<double>&,bool indpt);

    const uiWorldRect&	curView(bool indepth) const;

    uiFlatViewer*	viewer()		{ return vwr_; }

    Notifier<uiStratSynthDisp>	wvltChanged;
    Notifier<uiStratSynthDisp>	zoomChanged;
    Notifier<uiStratSynthDisp>	layerPropSelNeeded;
    Notifier<uiStratSynthDisp>	modSelChanged;

    mDeclInstanceCreatedNotifierAccess(uiStratSynthDisp);
    void		addTool(const uiToolButtonSetup&);
    void		addViewerToControl(uiFlatViewer&);

    void		modelChanged();
    bool		haveUserScaleWavelet();
    void		displaySynthetic(const SyntheticData*);

    uiMultiFlatViewControl* control() 	{ return control_; }

protected:

    int			longestaimdl_;
    StratSynth&		stratsynth_;
    const Strat::LayerModel& lm_;
    int			selectedtrace_;
    int			dispeach_;
    bool		dispflattened_;

    const ObjectSet<const TimeDepthModel>* d2tmodels_;
    SyntheticData* 	currentsynthetic_;

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
    uiLabeledComboBox*	datalist_;
    uiLabeledComboBox*	levelsnapselfld_;
    uiSynthGenDlg*	synthgendlg_;
    uiSynthSlicePos*	offsetposfld_;
    uiSynthSlicePos*	modelposfld_;
    uiFlatViewMainWin*	prestackwin_;

    void		setCurrentSynthetic();
    void		cleanSynthetics();
    void		doModelChange();
    const SeisTrcBuf&	curTrcBuf() const;
    void		updateSyntheticList();

    void		drawLevel();
    void		displayPreStackDirSynthetic(const SyntheticData*);
    void		displayPostStackDirSynthetic(const SyntheticData*);

    void		addEditSynth(CallBacker*);
    void		dataSetSel(CallBacker*);
    void		levelSnapChanged(CallBacker*);
    void		layerPropsPush(CallBacker*);
    void		offsetChged(CallBacker*);
    void		syntheticDataParChged(CallBacker*);
    void		modelPosChged(CallBacker*);
    void		scalePush(CallBacker*);
    void 		genNewSynthetic(CallBacker*);
    void		viewPreStackPush(CallBacker*);
    void		wvltChg(CallBacker*);
    void		zoomChg(CallBacker*);
    void		syntheticRemoved(CallBacker*);
    void		syntheticChanged(CallBacker*);

};


mClass(uiWellAttrib) uiSynthSlicePos : public uiGroup
{
public:
    			uiSynthSlicePos(uiParent*,const char* lbltxt);

    Notifier<uiSynthSlicePos>	positionChg;
    void		setLimitSampling(StepInterval<float>);
    int			getValue() const;

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


mClass(uiWellAttrib) uiStackGrp : public uiGroup
{
public:
    			uiStackGrp(uiParent*);

    Notifier<uiStackGrp> rangeChg;

    void		setLimitRange(Interval<float>);
    const Interval<float>& getRange() const;

protected:
    uiGenInput*		offsetfld_;
    Interval<float>	limitrg_;
    Interval<float>	offsetrg_;

    void		valChgCB( CallBacker* );
};


mClass(uiWellAttrib) uiSynthGenDlg : public uiDialog
{
public:
				uiSynthGenDlg(uiParent*,StratSynth&);

    void			getFromScreen();
    void			putToScreen();
    void			updateSynthNames();
    void			updateWaveletName();

    Notifier<uiSynthGenDlg>	genNewReq;
    CNotifier<uiSynthGenDlg,BufferString> synthRemoved;
    CNotifier<uiSynthGenDlg,BufferString> synthChanged;

protected:

    uiGenInput*			typefld_;
    uiGenInput*  		namefld_;
    uiCheckBox*			nmobox_;
    uiCheckBox*			stackfld_;
    uiRayTracerSel*		rtsel_;
    uiPushButton*		gennewbut_;
    uiPushButton*		applybut_;
    uiPushButton*		revertbut_;
    uiPushButton*		savebut_;
    uiListBox*			synthnmlb_;
    StratSynth&			stratsynth_;


    void			typeChg(CallBacker*);
    bool			genNewCB(CallBacker*);
    bool			acceptOK(CallBacker*);
    void			removeSyntheticsCB(CallBacker*);
    void			changeSyntheticsCB(CallBacker*);
    void			offsetChanged(CallBacker*);
    void			nameChanged(CallBacker*);

};

#endif

