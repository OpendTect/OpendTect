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
#include "stratlevel.h"
class CtxtIOObj;
class Wavelet;
class SeisTrcBuf;
class PropertyRef;
class TimeDepthModel;
class TimeDepthModelSet;
class PropertyRefSelection;
class ElasticPropSelection;
class uiToolBar;
class uiStratSynthDisp;
class uiStratLayerModelDisp;
class uiLayerSequenceGenDesc;
class uiStratGenDescTools;
class uiStratLayModEditTools;
class uiStratLayerModelLMSet;
class uiStratLayerModelManager;
namespace Strat
{
    class LayerModel; class LayerModelSuite;
    class LayerSequenceGenDesc;
}
namespace StratSynth { class DataMgr; }


mExpClass(uiWellAttrib) uiStratLayerModel : public uiMainWin
{ mODTextTranslationClass(uiStratLayerModel);
public:

    typedef Strat::LayerSequenceGenDesc	LayerSequenceGenDesc;
    typedef Strat::LayerModel		LayerModel;
    typedef Strat::LayerModelSuite	LayerModelSuite;
    typedef StratSynth::DataMgr		SynthDataMgr;

				uiStratLayerModel(uiParent*,
					const char* disptype=0,int opt=0);
				~uiStratLayerModel();

    void			go()		{ show(); }

    static const char*		sKeyModeler2Use();

    mDeclInstanceCreatedNotifierAccess(uiStratLayerModel);
    uiToolBar*			analysisToolBar()	   { return analtb_; }

    const LayerSequenceGenDesc&	genDesc() const		{ return desc_; }
    LayerModelSuite&		layerModelSuite()	{ return lms_; }
    const LayerModelSuite&	layerModelSuite() const	{ return lms_; }
    LayerModel&			layerModel();
    const LayerModel&		layerModel() const;
    const TimeDepthModelSet&	d2TModels() const;
    const SynthDataMgr&		synthDataMgr() const { return *synthdatamgr_; }
    Strat::Level::ID		curLevelID() const;
    DBKey			genDescID() const;
    bool			loadGenDesc(const DBKey&);
    void			setNrToGen(int);
    int				nrToGen() const;

    Notifier<uiStratLayerModel>			modelChange;
    CNotifier<uiStratLayerModel,IOPar*>		beforeSave;
    CNotifier<uiStratLayerModel,const IOPar*>	afterRetrieve;

    uiStratLayerModelDisp*      layModDisp() const	{ return moddisp_; }
    uiStratSynthDisp*		synthDisp() const	{ return synthdisp_; }

    bool			checkUnscaledWavelets();

protected:

    uiLayerSequenceGenDesc*	descdisp_;
    uiStratLayerModelDisp*	moddisp_	= 0;
    uiStratSynthDisp*		synthdisp_;
    uiStratGenDescTools*	gentools_;
    uiStratLayModEditTools*	modtools_;
    uiToolBar*			analtb_		= 0;

    LayerSequenceGenDesc&	desc_;
    LayerModelSuite&		lms_;
    RefMan<SynthDataMgr>	synthdatamgr_;
    ElasticPropSelection*	elpropsel_	= 0;
    mutable DirtyCountType	synthdatamgrdc_	= -1;

    CtxtIOObj&			descctio_;
    bool			needtoretrievefrpars_;
    int				nrmodels_;

    void			initWin(CallBacker*);
    void			setWinTitle();
    void			handleNewModel(LayerModel* mdl=0);

    void			setModelProps();
    void			setElasticProps();
    bool			openGenDesc();
    bool			saveGenDesc() const;
    bool			saveGenDescIfNecessary(
					bool allowcancel=true) const;
    bool			doLoadGenDesc();
    bool			doGenModels();
    bool			selectElasticProps(ElasticPropSelection&);
    void			updateDispEach(const LayerModel&);
    bool			closeOK();

    void			fillWorkBenchPars(IOPar&) const;

    void			snapshotCB(CallBacker*);
    void			xPlotReq(CallBacker*);
    void			helpCB(CallBacker*);
    void			openGenDescCB(CallBacker*) { openGenDesc(); }
    void			saveGenDescCB(CallBacker*) { saveGenDesc(); }
    void			manPropsCB(CallBacker*);
    void			genModelsCB(CallBacker*);
    void			selectElasticPropsCB(CallBacker*);
    void			synthInfoChangedCB(CallBacker*);
    void			modChgCB(CallBacker*);
    void			seqsAddedCB(CallBacker*);
    void			seqSelCB(CallBacker*);
    void			modInfoChangedCB(CallBacker*);

    friend class		uiStratLayerModelManager;

};
