#ifndef uistratsynthdisp_h
#define uistratsynthdisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id: uistratsynthdisp.h,v 1.19 2011-04-04 15:14:52 cvsbruno Exp $
________________________________________________________________________

-*/

#include "cubesampling.h"
#include "samplingdata.h"
#include "raytrace1d.h"
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
class uiFlatViewer;
class uiPushButton;
class uiRayTrcParamsDlg;
class uiSeisWaveletSel;
class uiOffsetSlicePos;
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

    uiRayTrcParamsDlg*	raytrcpardlg_;
    Seis::RaySynthGenerator::RayParams raypars_;
    uiOffsetSlicePos*	posfld_;

    void		doModelChange();
    void		rayTrcParPush(CallBacker*);
    void		rayTrcParChged(CallBacker*);
    void		wvltChg(CallBacker*);
    void		scalePush(CallBacker*);
    void		zoomChg(CallBacker*);
    int			getVelIdx(bool&) const;
    int			getDenIdx(bool&) const;
};


mClass uiOffsetSlicePos : public uiSlicePos2DView
{
public:
    			uiOffsetSlicePos(uiParent*);

    uiGroup*		attachGrp() 	{ return attachgrp_; }

protected:
    uiGroup*		attachgrp_;
};


mClass uiRayTrcParamsDlg : public uiDialog
{
public:
				uiRayTrcParamsDlg(uiParent*,
					Seis::RaySynthGenerator::RayParams&);

    void			setLimitSampling(const CubeSampling& cs);
    Notifier<uiRayTrcParamsDlg>  parChged;

protected:

    Seis::RaySynthGenerator::RayParams& raypars_;
    uiComboBox*			directionfld_;
    CubeSampling		limitcs_;

    uiGenInput*			offsetfld_;
    uiGenInput*			offsetstepfld_;
    uiGenInput*			sourcerecfld_;
    uiGenInput*			vp2vsfld_;

    void 			dirChg(CallBacker*);
    void 			parChg(CallBacker*);
};


#endif
