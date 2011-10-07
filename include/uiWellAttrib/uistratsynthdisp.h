#ifndef uistratsynthdisp_h
#define uistratsynthdisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id: uistratsynthdisp.h,v 1.38 2011-10-07 15:10:10 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "uimainwin.h"
#include "uidialog.h"
#include "uiflatviewslicepos.h"
#include "stratsynth.h"

class FlatDataPack;
class TimeDepthModel;
class SeisTrcBuf;
class Wavelet;
class RayParam;
class uiComboBox;
class uiGenInput;
class uiCheckBox;
class uiFlatViewer;
class uiRayTracer1D;
class uiLabeledComboBox;
class uiFlatViewMainWin;
class uiOffsetSlicePos;
class uiPushButton;
class uiRayTrcParamsDlg;
class uiSeisWaveletSel;
class uiToolButton;
class uiToolButtonSetup;
namespace Strat { class LayerModel; }


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


mClass uiRayTrcParamsDlg : public uiDialog
{
public:
				uiRayTrcParamsDlg(uiParent*,RayParams&);
protected:
    uiCheckBox*			nmobox_;
    uiGenInput*			stackfld_;
    uiRayTracer1D*		raytrace1dgrp_;

    RayParams&			raypars_;

    bool			acceptOK( CallBacker* );
};


mClass uiStratSynthDisp : public uiGroup
{
public:

    			uiStratSynthDisp(uiParent*,const Strat::LayerModel&);
    			~uiStratSynthDisp();

    const Strat::LayerModel& layerModel() const;	
    const char*		levelName() const	{ return levelname_; }
    const MultiID&	waveletID() const;
    const ObjectSet<SyntheticData>& getSynthetics() const;

    void		setDispMrkrs(const char* lvlnm,const TypeSet<float>&,
	    			     Color);
    const uiWorldRect&	curView(bool indepth) const;

    Notifier<uiStratSynthDisp>	wvltChanged;
    Notifier<uiStratSynthDisp>	zoomChanged;
    Notifier<uiStratSynthDisp>	layerPropSelNeeded;
    Notifier<uiStratSynthDisp>	modSelChanged;

    mDeclInstanceCreatedNotifierAccess(uiStratSynthDisp);
    void		addTool(const uiToolButtonSetup&);

    void		modelChanged();


protected:

    BufferString	levelname_;
    int			longestaimdl_;
    StratSynth&		stratsynth_;
    const ObjectSet<const TimeDepthModel>* d2tmodels_;
    SyntheticData* 	currentsynthetic_;

    uiGroup*		topgrp_;
    uiGroup*		modelgrp_;
    uiSeisWaveletSel*	wvltfld_;
    uiFlatViewer*	vwr_;
    uiPushButton*	scalebut_;
    uiToolButton*	lasttool_;
    uiLabeledComboBox*	modellist_;
    uiRayTrcParamsDlg*	raytrcpardlg_;
    uiSynthSlicePos*	offsetposfld_;
    uiSynthSlicePos*	modelposfld_;
    uiFlatViewMainWin*	prestackwin_;

    void		cleanSynthetics();
    void		doModelChange();
    const SeisTrcBuf&	curTrcBuf() const;

    void		displaySynthetic(const SyntheticData*);
    void		displayPreStackSynthetic(const SyntheticData*);
    void		displayPostStackSynthetic(const SyntheticData*);
    void		addSynthetic(const RayParams& rp,bool isps);
    BufferString	getSynthDefaultName(const RayParams&) const;

    void		addSynth2List(CallBacker*);
    void		dataSetSel(CallBacker*);
    void		layerPropsPush(CallBacker*);
    void		rayTrcParPush(CallBacker*);
    void		rayTrcParChged(CallBacker*);
    void		offsetPosChged(CallBacker*);
    void		modelPosChged(CallBacker*);
    void		scalePush(CallBacker*);
    void		viewPreStackPush(CallBacker*);
    void		wvltChg(CallBacker*);
    void		zoomChg(CallBacker*);

};

#endif
