#ifndef uistratsynthdisp_h
#define uistratsynthdisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id: uistratsynthdisp.h,v 1.18 2011-04-01 12:59:18 cvsbruno Exp $
________________________________________________________________________

-*/

#include "cubesampling.h"
#include "samplingdata.h"
#include "raytrace1d.h"
#include "uigroup.h"
#include "uidialog.h"
#include "datapack.h"

class FlatDataPack;
class TimeDepthModel;
class SeisTrcBuf;
class Wavelet;
class uiComboBox;
class uiGenInput;
class uiFlatViewer;
class uiPushButton;
class uiRayTrcSetupDlg;
class uiSeisWaveletSel;
class uiSlicePos2DView;
class uiToolButton;
class uiToolButtonSetup;
namespace Strat { class LayerModel; }


mClass uiStratSynthDisp : public uiGroup
{
public:

    			uiStratSynthDisp(uiParent*,const Strat::LayerModel&);
    			~uiStratSynthDisp();

    void		setDispMrkrs(const char* lvlnm,const TypeSet<float>&,
	    			     Color);
    void		modelChanged();

    const uiWorldRect&	curView(bool indepth) const;
    const SeisTrcBuf&	curTrcBuf() const;
    const FlatDataPack*	dataPack() const;		//!< may return null
    DataPack::FullID	packID() const;
    const ObjectSet<TimeDepthModel>& d2TModels() const	{ return d2tmodels_; }

    Notifier<uiStratSynthDisp>	wvltChanged;
    Notifier<uiStratSynthDisp>	zoomChanged;

    static Notifier<uiStratSynthDisp>&	fieldsCreated();
    void		addTool(const uiToolButtonSetup&);

protected:

    const Strat::LayerModel& lm_;
    Wavelet*		wvlt_;
    BufferString	levelname_;
    int			longestaimdl_;
    ObjectSet<TimeDepthModel> d2tmodels_;

    uiGroup*		topgrp_;
    uiSeisWaveletSel*	wvltfld_;
    uiFlatViewer*	vwr_;
    uiPushButton*	scalebut_;
    uiToolButton*	lasttool_;
    uiRayTrcSetupDlg*	raytrcpardlg_;
    RayTracer1D::Setup	raytrcsetup_;
    CubeSampling	cs_;

    void		rayTrcParPush(CallBacker*);
    void		rayTrcParChged(CallBacker*);
    void		wvltChg(CallBacker*);
    void		scalePush(CallBacker*);
    void		zoomChg(CallBacker*);
    int			getVelIdx(bool&) const;
    int			getDenIdx(bool&) const;
};


mClass uiRayTrcSetupDlg : public uiDialog
{
public:
				uiRayTrcSetupDlg(uiParent*,
						RayTracer1D::Setup&,
						CubeSampling&);

    const SamplingData<float>&	offsetRange() const 	{ return offsetrg_; }

    Notifier<uiRayTrcSetupDlg>  parChged;

protected:
    RayTracer1D::Setup&		rtsetup_;
    SamplingData<float>		offsetrg_;
    CubeSampling&		cs_;
    uiComboBox*			directionfld_;

    uiSlicePos2DView*		posfld_;
    uiGenInput*			offsetfld_;
    uiGenInput*			offsetstepfld_;
    uiGenInput*			sourcerecfld_;
    uiGenInput*			vp2vsfld_;

    void 			dirChg(CallBacker*);
    void 			parChg(CallBacker*);
};



#endif
