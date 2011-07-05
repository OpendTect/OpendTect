#ifndef uistratsynthdisp_h
#define uistratsynthdisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id: uistratsynthdisp.h,v 1.27 2011-07-05 08:25:19 cvsbruno Exp $
________________________________________________________________________

-*/

#include "cubesampling.h"
#include "samplingdata.h"
#include "uigroup.h"
#include "uimainwin.h"
#include "uidialog.h"
#include "uiflatviewslicepos.h"
#include "datapack.h"
#include "synthseis.h"

class FlatDataPack;
class TimeDepthModel;
class SeisTrcBuf;
class Wavelet;
class uiComboBox;
class uiGenInput;
class uiCheckBox;
class uiFlatViewer;
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


mStruct RayParams
{				RayParams()
				    : usenmotimes_(true)
				    , dostack_(false)
				{
				    cs_.hrg.setInlRange(Interval<int>(1,1));
				    cs_.hrg.setCrlRange(Interval<int>(0,0));
				    cs_.hrg.step = BinID( 1, 100 );
				    cs_.zrg.set( 0, 0, 0  );
				}
    CubeSampling 		cs_; //inl are models, crl are offsets
    bool			usenmotimes_;
    bool			dostack_;
    RayTracer1D::Setup		setup_;
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

    uiGenInput*			offsetfld_;
    uiGenInput*			offsetstepfld_;
    uiGenInput*			sourcerecfld_;
    uiGenInput*			vp2vsfld_;
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


mClass SyntheticData 
{
public:
				SyntheticData(const char* name)
				    : packid_(DataPack::cNoID())
				    {}

    DataPack::FullID 		packid_;
    bool			isps_;
    ObjectSet<const TimeDepthModel> d2tmodels_;
    const Wavelet*		wvlt_;
};




mClass uiStratSynthDisp : public uiGroup
{
public:

    			uiStratSynthDisp(uiParent*,const Strat::LayerModel&);
    			~uiStratSynthDisp();

    void		setDispMrkrs(const char* lvlnm,const TypeSet<float>&,
	    			     Color);
    const uiWorldRect&	curView(bool indepth) const;
    const SeisTrcBuf&	curTrcBuf() const;
    const FlatDataPack* dataPack() const;
    DataPack::FullID 	packID() const;

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

    ObjectSet<const TimeDepthModel> d2tmodels_;
    RayParams 		raypars_;

    ObjectSet<const SyntheticData> synthetics_;

    uiGroup*		topgrp_;
    uiGroup*		modelgrp_;
    uiSeisWaveletSel*	wvltfld_;
    uiFlatViewer*	vwr_;
    uiPushButton*	scalebut_;
    uiToolButton*	lasttool_;
    uiLabeledComboBox*	modellist_;
    uiRayTrcParamsDlg*	raytrcpardlg_;
    uiOffsetSlicePos*	posfld_;

    void		cleanSynthetics();
    void		doModelChange();
    const CubeSampling& getLimitSampling() const;

    int			getVelIdx(bool&) const;
    int			getDenIdx(bool&) const;

    DataPack*		genNewDataPack(const RayParams& raypars,
				    ObjectSet<const TimeDepthModel>& d2ts,
				    bool isps) const;

    void		addSynth2List(CallBacker*);
    void		dataSetSel(CallBacker*);
    void		rayTrcParPush(CallBacker*);
    void		rayTrcParChged(CallBacker*);
    void		rayTrcPosChged(CallBacker*);
    void		scalePush(CallBacker*);
    void		wvltChg(CallBacker*);
    void		zoomChg(CallBacker*);
};

#endif
