#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/

#include "uiwellattribmod.h"

#include "uimainwin.h"
#include "uistring.h"

class CtxtIOObj;
class ElasticPropSelection;
class PropertyRef;
class PropertyRefSelection;
class SeisTrcBuf;
class StratSynth;
class SyntheticData;
class TimeDepthModel;
class Wavelet;
namespace Strat
{
    class LayerModel;
    class LayerModelProviderImpl;
    class LayerSequenceGenDesc;
}

class uiLayerSequenceGenDesc;
class uiStratGenDescTools;
class uiStratLayerModelDisp;
class uiStratLayModEditTools;
class uiStratSynthCrossplot;
class uiStratSynthDisp;
class uiToolBar;


mExpClass(uiWellAttrib) uiStratLayerModel : public uiMainWin
{ mODTextTranslationClass(uiStratLayerModel);
public:
    friend class		uiStratLayerModelManager;

				uiStratLayerModel(uiParent*,
					const char* disptype=0,int opt=0);
				~uiStratLayerModel();

    void			go()		{ show(); }

    static const char*		sKeyModeler2Use();
    static void			initClass();

    mDeclInstanceCreatedNotifierAccess(uiStratLayerModel);
    uiToolBar*			analysisToolBar()	   { return analtb_; }

    void				setNrModels(int);
    int					nrModels() const;
    const Strat::LayerSequenceGenDesc&	genDesc() const	   { return desc_; }
    const Strat::LayerModel&		layerModelOriginal() const;
    Strat::LayerModel&			layerModelOriginal();
    const Strat::LayerModel&		layerModelEdited() const;
    Strat::LayerModel&			layerModelEdited();
    const Strat::LayerModel&            layerModel() const;
    Strat::LayerModel&                  layerModel();
    const char*				levelName() const; //!< null if none
    const StratSynth&			currentStratSynth() const;
    StratSynth&				currentStratSynth();
    const StratSynth&			normalStratSynth() const;
    const StratSynth&			editStratSynth() const;
    bool				isEditUsed() const;
    const PropertyRefSelection& modelProperties() const;
    const ObjectSet<const TimeDepthModel>& d2TModels() const;
    const Wavelet*			wavelet() const;
    MultiID				genDescID() const;

    Notifier<uiStratLayerModel>	newModels;
    Notifier<uiStratLayerModel>	waveletChanged;
    Notifier<uiStratLayerModel> saveRequired;   // CallBacker: CBCapsule<IOPar>
    Notifier<uiStratLayerModel> retrieveRequired;// CallBacker: CBCapsule<IOPar>

    bool			checkUnscaledWavelet();

    mDeprecated("Provide a model name")
    static void			doBasicLayerModel();
    mDeprecated("Provide a model name")
    static void			doLayerModel(const char* modnm,int opt=0);

    static void			doBasicLayerModel(uiParent*);
    static void			doLayerModel(uiParent*,const char* modnm,
								int opt=0);
    static uiStratLayerModel*	getUILayerModel();

    void			displayFRText(bool yn,bool isbrine=true);
    uiStratLayerModelDisp*      getLayModelDisp() const	{ return moddisp_; }
    void			displayFRResult(bool usefr,bool parschanged,
						bool isbrine);
    void			prepareFluidRepl();
    void			resetFluidRepl();

    void			setSynthView(const uiWorldRect& wr);
    const uiWorldRect&		curSynthView() const;

protected:

    uiLayerSequenceGenDesc*	seqdisp_;
    uiStratLayerModelDisp*	moddisp_;
    uiStratSynthDisp*		synthdisp_;
    uiStratGenDescTools*	gentools_;
    uiStratLayModEditTools*	modtools_;
    uiToolBar*			analtb_;

    Strat::LayerSequenceGenDesc& desc_;
    Strat::LayerModelProviderImpl& lmp_;
    CtxtIOObj&			descctio_;
    ElasticPropSelection*	elpropsel_;
    /*mDeprecated*/ bool	mostlyfilledwithbrine_;
    bool			needtoretrievefrpars_;
    bool			automksynth_;

    bool			canShowFlattened() const;
    void			setWinTitle();
    void			handleNewModel();
    void			setModelProps();
    void			setElasticProps();
    bool			selElasticProps(ElasticPropSelection&);
    bool			openGenDesc();
    bool			saveGenDesc() const;
    bool			saveGenDescIfNecessary(
					bool allowcancel=true) const;
    void			doGenModels(bool forceupdsynth,
					    bool overridedispeach=false);
    void			calcAndSetDisplayEach(bool overridepar);
    bool			closeOK();

    void			fillDisplayPars(IOPar&) const;
    void			fillWorkBenchPars(IOPar&) const;
    void			fillSyntheticsPars(IOPar&) const;
    bool			useDisplayPars(const IOPar&);
    bool			useSyntheticsPars(const IOPar&);


    void			openGenDescCB(CallBacker*) { openGenDesc(); }
    void			saveGenDescCB(CallBacker*) { saveGenDesc(); }
    void			manPropsCB(CallBacker*);
    void			snapshotCB(CallBacker*);
    void			synthDispParsChangedCB(CallBacker*);
    void			lmDispParsChangedCB(CallBacker*);
    void			selPropChgCB(CallBacker*);
    void			infoChanged(CallBacker*);
    void			selElasticPropsCB(CallBacker*);

    void			initWin(CallBacker*);
    void			dispEachChg(CallBacker*);
    void			mkSynthChg(CallBacker*);
    void			levelChg(CallBacker*);
    void			flattenChg(CallBacker*);
    void			lmViewChangedCB(CallBacker*);
    void			seqSel(CallBacker*);
    void			modEd(CallBacker*);
    void			modDispRangeChanged(CallBacker*);
    void			syntheticsChangedCB(CallBacker*);
    void			viewChgedCB(CallBacker*);
    void			wvltChg(CallBacker*);
    void			modSelChg(CallBacker*);
    void			genModels(CallBacker*);
    void			xPlotReq(CallBacker*);
    void			nrModelsChangedCB(CallBacker*);
    void			helpCB(CallBacker*);

public:
    uiStratSynthDisp*		getSynthDisp() const	{ return synthdisp_; }
};
