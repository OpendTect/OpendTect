#ifndef uistratsynthdisp_h
#define uistratsynthdisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id: uistratsynthdisp.h,v 1.30 2011-07-25 15:07:49 cvsbruno Exp $
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
class uiComboBox;
class uiGenInput;
class uiCheckBox;
class uiFlatViewer;
class uiElasticPropSelDlg;
class uiRayTracer1D;
class uiLabeledComboBox;
class uiOffsetSlicePos;
class uiPushButton;
class uiRayTrcParamsDlg;
class uiSeisWaveletSel;
class uiToolButton;
class uiToolButtonSetup;
namespace Strat { class LayerModel; }


mClass uiOffsetSlicePos : public uiSlicePos2DView
{
public:
    			uiOffsetSlicePos(uiParent*);

    uiGroup*		attachGrp() 	{ return attachgrp_; }
protected:
    uiGroup*		attachgrp_;
};


mClass uiRayTrcParamsGrp : public uiGroup
{
public:

    mClass Setup
    {
	public:
				Setup(RayParams& rpars)
				    : raypars_(rpars) 
				    , offsetdir_(false)
				    , withraysettings_(true)
				    {}

	mDefSetupMemb(RayParams&,raypars)
	mDefSetupMemb(bool,offsetdir)
	mDefSetupMemb(bool,withraysettings)
    };

				uiRayTrcParamsGrp(uiParent*,const Setup&);

    void			setLimitSampling(const CubeSampling&);
    void			setOffSetDirection(bool yn) 
    				{ isoffsetdir_=yn; updateCB(0); }

    void			doUpdate() 	{ updateCB(0); }
    
protected:

    RayParams& 			raypars_;

    uiRayTracer1D*		raytrace1dgrp_;

    uiCheckBox*			nmobox_;
    uiGenInput*			stackfld_;
    CubeSampling		limitcs_;

    bool			isoffsetdir_;
    bool			previsoffsetdir_;

    void 			updateCB(CallBacker*);
};


mClass uiRayTrcParamsDlg : public uiDialog
{
public:
				uiRayTrcParamsDlg(uiParent*,RayParams&);

    void			setLimitSampling(const CubeSampling& cs);
    uiRayTrcParamsGrp*		parsGrp() 	{ return raytrcpargrp_; }

protected:

    uiComboBox*			directionfld_;
    uiRayTrcParamsGrp*		raytrcpargrp_;

    void 			dirChg(CallBacker*);
    bool			acceptOK(CallBacker*);
};


mClass uiStratSynthDisp : public uiGroup
{
public:

    			uiStratSynthDisp(uiParent*,const Strat::LayerModel&);
    			~uiStratSynthDisp();

    void		setDispMrkrs(const char* lvlnm,const TypeSet<float>&,
	    			     Color);
    const uiWorldRect&	curView(bool indepth) const;

    Notifier<uiStratSynthDisp>	wvltChanged;
    Notifier<uiStratSynthDisp>	zoomChanged;

    static Notifier<uiStratSynthDisp>&	fieldsCreated();
    void		addTool(const uiToolButtonSetup&);

    void		modelChanged();

    const ObjectSet<const SyntheticData>& getSynthetics() const 
    					{ return synthetics_;}
protected:

    const Strat::LayerModel& lm_;
    Wavelet*		wvlt_;
    BufferString	levelname_;
    int			longestaimdl_;

    RayParams& 		raypars_;

    const ObjectSet<const TimeDepthModel>* d2tmodels_;
    const SyntheticData* tmpsynthetic_;

    ObjectSet<const SyntheticData> synthetics_;

    uiGroup*		topgrp_;
    uiGroup*		modelgrp_;
    uiSeisWaveletSel*	wvltfld_;
    uiFlatViewer*	vwr_;
    uiPushButton*	scalebut_;
    uiToolButton*	lasttool_;
    uiLabeledComboBox*	modellist_;
    uiElasticPropSelDlg* layerpropseldlg_;
    uiRayTrcParamsDlg*	raytrcpardlg_;
    uiOffsetSlicePos*	posfld_;
    StratSynth&		stratsynth_;

    void		cleanSynthetics();
    void		doModelChange();
    const CubeSampling& getLimitSampling() const;
    const SeisTrcBuf&	curTrcBuf() const;

    void		displaySynthetics(const SyntheticData*);

    void		addSynth2List(CallBacker*);
    void		dataSetSel(CallBacker*);
    void		layerPropsPush(CallBacker*);
    void		rayTrcParPush(CallBacker*);
    void		rayTrcParChged(CallBacker*);
    void		rayTrcPosChged(CallBacker*);
    void		scalePush(CallBacker*);
    void		wvltChg(CallBacker*);
    void		zoomChg(CallBacker*);
};

#endif
