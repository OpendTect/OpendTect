#ifndef uistratlayermodel_h
#define uistratlayermodel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
 RCS:           $Id: uistratlayermodel.h,v 1.32 2012-08-29 14:21:41 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uimainwin.h"
class CtxtIOObj;
class Wavelet;
class SeisTrcBuf;
class SyntheticData;
class TimeDepthModel;
class ElasticPropSelection;
class uiGenInput;
class uiSpinBox;
class uiToolBar;
class uiStratSynthDisp;
class uiStratLayerModelDisp;
class uiLayerSequenceGenDesc;
class uiStratGenDescTools;
class uiStratLayModEditTools;
namespace Strat { class LayerModel; class LayerSequenceGenDesc; }


mClass(uiWellAttrib) uiStratLayerModel : public uiMainWin
{
public:

				uiStratLayerModel(uiParent*,
						  const char* seqdisptype=0);
				~uiStratLayerModel();

    void			go()		{ show(); }

    static const char*		sKeyModeler2Use();

    mDeclInstanceCreatedNotifierAccess(uiStratLayerModel);
    uiToolBar*			analysisToolBar()	   { return analtb_; }

    const Strat::LayerSequenceGenDesc&	genDesc() const	   { return desc_; }
    const Strat::LayerModel&		layerModel() const { return modl_; }
    const char*				levelName() const; //!< null if none
    const SeisTrcBuf&			postStackTraces() const;
    const ObjectSet<const TimeDepthModel>& d2TModels() const;
    const Wavelet*			wavelet() const;
    MultiID				genDescID() const;

    Notifier<uiStratLayerModel>	newModels;
    Notifier<uiStratLayerModel>	levelChanged;
    Notifier<uiStratLayerModel>	waveletChanged;

    bool			checkUnscaledWavelet();

    static void			doBasicLayerModel();
    static void			doLayerModel(const char* modnm);

    uiStratLayerModelDisp*      getLayModelDisp() const	{ return moddisp_; }
    void			displayFRResult( uiStratLayerModelDisp*,
	   					 SyntheticData* );

    //Utility
    SyntheticData*		getCurrentSyntheticData() const;

protected:

    uiLayerSequenceGenDesc*	seqdisp_;
    uiStratLayerModelDisp*	moddisp_;
    uiStratSynthDisp*		synthdisp_;
    uiStratGenDescTools*	gentools_;
    uiStratLayModEditTools*	modtools_;
    uiToolBar*			analtb_;

    Strat::LayerSequenceGenDesc& desc_;
    Strat::LayerModel&		modl_;
    CtxtIOObj&			descctio_;
    ElasticPropSelection*	elpropsel_;

    void			initWin(CallBacker*);
    void			dispEachChg(CallBacker*);
    void			levelChg(CallBacker*);
    void			modDispRangeChanged(CallBacker*);
    void			zoomChg(CallBacker*);
    void			wvltChg(CallBacker*);
    void			modSelChg(CallBacker*);
    void			genModels(CallBacker*);
    void			xPlotReq(CallBacker*);

    void			setWinTitle();
    void			setModelProps();
    void			setElasticProps();
    void			selElasticPropsCB(CallBacker*);
    bool			selElasticProps(ElasticPropSelection&);
    void			openGenDescCB(CallBacker*) { openGenDesc(); }
    bool			openGenDesc();
    void			saveGenDescCB(CallBacker*) { saveGenDesc(); }
    bool			saveGenDesc() const;
    bool			saveGenDescIfNecessary() const;
    void			manPropsCB(CallBacker*);

    bool			closeOK();

public:

    static void			initClass();

};


#endif

