#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellattribmod.h"

#include "uimainwin.h"
#include "uistring.h"
#include "stratlevel.h"

class CtxtIOObj;
class ElasticPropSelection;
class HelpKey;
class uiLayerSequenceGenDesc;
class uiStratGenDescTools;
class uiStratLayerModelDisp;
class uiStratLayerModelManager;
class uiStratLayModEditTools;
class uiStratSynthDisp;
class uiToolBar;

namespace Strat
{
    class LayerModel;
    class LayerModelSuite;
    class LayerSequenceGenDesc;
}
namespace StratSynth { class DataMgr; }


mExpClass(uiWellAttrib) uiStratLayerModel : public uiMainWin
{ mODTextTranslationClass(uiStratLayerModel);
public:

				uiStratLayerModel(uiParent*,
					const char* disptype=nullptr,int opt=0);
				~uiStratLayerModel();

    static const char*		sKeyModeler2Use();

    mDeclInstanceCreatedNotifierAccess(uiStratLayerModel);
    uiToolBar*			analysisToolBar()	   { return analtb_; }

    const Strat::LayerSequenceGenDesc&	genDesc() const	   { return desc_; }
    Strat::LayerModelSuite&		layerModelSuite()	{ return lms_; }
    const Strat::LayerModelSuite&	layerModelSuite() const { return lms_; }
    Strat::LayerModel&                  layerModel();
    const Strat::LayerModel&		layerModel() const;
    const StratSynth::DataMgr&		synthDataMgr() const
					{ return *synthdatamgr_.ptr(); }
    Strat::LevelID			curLevelID() const;
    MultiID				genDescID() const;
    bool				loadGenDesc(const MultiID&);
    void				setNrToGen(int);
    int					nrToGen() const;
    bool				checkUnscaledWavelets();

    CNotifier<uiStratLayerModel,IOPar*>		beforeSave;
    CNotifier<uiStratLayerModel,const IOPar*>	afterRetrieve;

    uiStratLayerModelDisp*		layModDisp() const { return moddisp_; }
    uiStratSynthDisp*			synthDisp() const  { return synthdisp_;}

    const HelpKey&			helpKey() const;
    void				setHelpKey(const HelpKey&);

protected:

    uiLayerSequenceGenDesc*	descdisp_;
    uiStratLayerModelDisp*	moddisp_	= nullptr;
    uiStratSynthDisp*		synthdisp_;
    uiStratGenDescTools*	gentools_;
    uiStratLayModEditTools*	modtools_;
    uiToolBar*			analtb_		= nullptr;

    Strat::LayerSequenceGenDesc& desc_;
    Strat::LayerModelSuite&	lms_;
    PtrMan<StratSynth::DataMgr> synthdatamgr_;
    mutable DirtyCountType	synthdatamgrdc_ = -1;

    CtxtIOObj&			descctio_;
    bool			needtoretrievefrpars_ = false;
    int				nrmodels_;

    void			initWin(CallBacker*);
    void			setWinTitle();
    void			handleNewModel(Strat::LayerModel* =nullptr,
					       bool full=false);

    void			setModelProps(const Strat::LayerModel&);
    bool			openGenDesc();
    bool			saveGenDesc() const;
    bool			saveGenDescIfNecessary(
					bool allowcancel=true) const;
    bool			doLoadGenDesc();
    bool			doGenModels(bool full=false);
    IOPar			getSynthPars() const;
    bool			updateDispEach(const Strat::LayerModel&);
    bool			closeOK() override;

    void			fillWorkBenchPars(IOPar&) const;

    void			snapshotCB(CallBacker*);
    void			xPlotReq(CallBacker*);
    void			helpCB(CallBacker*);
    void			openGenDescCB( CallBacker* ) { openGenDesc(); }
    void			saveGenDescCB( CallBacker* ) { saveGenDesc(); }
    void			manPropsCB(CallBacker*);
    void			genModelsCB(CallBacker*);
    void			elasticPropsCB(CallBacker*);
    void			synthInfoChangedCB(CallBacker*);
    void			modChgCB(CallBacker*);
    void			seqsAddedCB(CallBacker*);
    void			seqSelCB(CallBacker*);
    void			modInfoChangedCB(CallBacker*);

    friend class		uiStratLayerModelManager;

};
