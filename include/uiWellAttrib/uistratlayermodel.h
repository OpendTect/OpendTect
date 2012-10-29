#ifndef uistratlayermodel_h
#define uistratlayermodel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uimainwin.h"
class CtxtIOObj;
class Wavelet;
class SeisTrcBuf;
class PropertyRef;
class SyntheticData;
class TimeDepthModel;
class PropertyRefSelection;
class ElasticPropSelection;
class uiGenInput;
class uiSpinBox;
class uiToolBar;
class uiStratSynthDisp;
class uiStratLayerModelDisp;
class uiLayerSequenceGenDesc;
class uiStratGenDescTools;
class uiStratLayModEditTools;
class uiStratLayerModelLMProvider;
namespace Strat { class LayerModel; class LayerSequenceGenDesc; }


mClass uiStratLayerModel : public uiMainWin
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
    const Strat::LayerModel&		layerModelOriginal() const;
    Strat::LayerModel&			layerModelOriginal();
    const Strat::LayerModel&		layerModelEdited() const;
    Strat::LayerModel&			layerModelEdited();
    const Strat::LayerModel&            layerModel() const;
    Strat::LayerModel&                  layerModel();
    const char*				levelName() const; //!< null if none
    const SeisTrcBuf&			postStackTraces() const;
    const SeisTrcBuf&			modelTraces(const PropertyRef&) const;
    const PropertyRefSelection&		modelProperties() const;
    const ObjectSet<const TimeDepthModel>& d2TModels() const;
    const Wavelet*			wavelet() const;
    MultiID				genDescID() const;

    Notifier<uiStratLayerModel>	newModels;
    Notifier<uiStratLayerModel>	levelChanged;
    Notifier<uiStratLayerModel>	waveletChanged;

    bool			checkUnscaledWavelet();

protected:

    uiLayerSequenceGenDesc*	seqdisp_;
    uiStratLayerModelDisp*	moddisp_;
    uiStratSynthDisp*		synthdisp_;
    uiStratGenDescTools*	gentools_;
    uiStratLayModEditTools*	modtools_;
    uiToolBar*			analtb_;

    Strat::LayerSequenceGenDesc& desc_;
    uiStratLayerModelLMProvider& lmp_;
    CtxtIOObj&			descctio_;
    ElasticPropSelection*	elpropsel_;
    bool			usepostfrmodl_;

    void			initWin(CallBacker*);
    void			dispEachChg(CallBacker*);
    void			levelChg(CallBacker*);
    void			seqSel(CallBacker*);
    void			modEd(CallBacker*);
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
    static void			doBasicLayerModel();
    static void			doLayerModel(const char* modnm);

    uiStratLayerModelDisp*      getLayModelDisp() const { return moddisp_; }
    void                        displayFRResult( SyntheticData* );
    void			prepareFluidRepl();
//Utility
    SyntheticData*		getCurrentSyntheticData() const;

    // CallBacker: CBCapsule<IOPar>
    Notifier<uiStratLayerModel>* saveRequiredNotif();
    Notifier<uiStratLayerModel>* retrieveRequiredNotif();

protected:
    void			fillDisplayPars(IOPar&) const;
    void			fillWorkBenchPars(IOPar&) const;
    bool			useDisplayPars(const IOPar&);
};


#endif
