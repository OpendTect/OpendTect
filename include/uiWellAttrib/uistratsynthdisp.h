#ifndef uistratsynthdisp_h
#define uistratsynthdisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id: uistratsynthdisp.h,v 1.52 2012-07-17 15:16:50 cvsbruno Exp $
________________________________________________________________________

-*/

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
class uiLabeledComboBox;
class uiFlatViewMainWin;
class uiMultiFlatViewControl;
class uiOffsetSlicePos;
class uiPushButton;
class uiRayTrcParamsDlg;
class uiSeisWaveletSel;
class uiStackGrp;
class uiSynthSlicePos;
class uiToolButton;
class uiToolButtonSetup;
namespace Strat { class LayerModel; }


mClass uiStratSynthDisp : public uiGroup
{
public:

    			uiStratSynthDisp(uiParent*,const Strat::LayerModel&);
    			~uiStratSynthDisp();

    const Strat::LayerModel& layerModel() const;	
    const char*		levelName() const;
    const MultiID&	waveletID() const;
    const Wavelet*	getWavelet() const;

    const ObjectSet<SyntheticData>& getSynthetics() const;
    void		genSyntheticsFor(const Strat::LayerModel&,
	    				SeisTrcBuf&);
    const SeisTrcBuf&	postStackTraces() const;
    const ObjectSet<const TimeDepthModel>* d2TModels() const;

    void		setDispMrkrs(const char* lvlnm,const TypeSet<float>&,
	    			     Color);
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

protected:

    int			longestaimdl_;
    StratSynth&		stratsynth_;
    const Strat::LayerModel& lm_;
    int			dispeach_;

    const ObjectSet<const TimeDepthModel>* d2tmodels_;
    SyntheticData* 	currentsynthetic_;

    uiMultiFlatViewControl* control_;

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
    uiCheckBox*		stackbox_;
    uiRayTrcParamsDlg*	raytrcpardlg_;
    uiSynthSlicePos*	offsetposfld_;
    uiSynthSlicePos*	modelposfld_;
    uiStackGrp*		stackfld_;
    uiFlatViewMainWin*	prestackwin_;

    void		cleanSynthetics();
    void		doModelChange();
    const SeisTrcBuf&	curTrcBuf() const;

    void		drawLevel();
    void		displaySynthetic(const SyntheticData*);
    void		displayPreStackSynthetic(const SyntheticData*);
    void		displayPostStackSynthetic(const SyntheticData*);

    void		addEditSynth(CallBacker*);
    void		dataSetSel(CallBacker*);
    void		levelSnapChanged(CallBacker*);
    void		layerPropsPush(CallBacker*);
    void		offsetChged(CallBacker*);
    void		rayTrcParChged(CallBacker*);
    void		modelPosChged(CallBacker*);
    void		scalePush(CallBacker*);
    void		viewPreStackPush(CallBacker*);
    void		wvltChg(CallBacker*);
    void		zoomChg(CallBacker*);

};


mClass uiSynthSlicePos : public uiGroup
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


mClass uiStackGrp : public uiGroup
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


mClass uiRayTrcParamsDlg : public uiDialog
{
public:
				uiRayTrcParamsDlg(uiParent*,IOPar&);
protected:
    uiCheckBox*			nmobox_;
    uiRayTracerSel*		rtsel_;
    IOPar&			raypars_;

    bool			acceptOK( CallBacker* );
};



#endif
