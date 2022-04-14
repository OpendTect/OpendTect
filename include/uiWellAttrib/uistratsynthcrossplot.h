#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
________________________________________________________________________

-*/

#include "uiwellattribmod.h"

#include "uidialog.h"
#include "datapack.h"
#include "stratlevel.h"
#include "uistring.h"

class DataPointSet;
class SeisTrcBuf;
class SeisTrcBufDataPack;
class SeisTrcInfo;
class SyntheticData;
class TimeDepthModel;
class uiStratSeisEvent;
class uiAttribDescSetBuild;
class uiStratLaySeqAttribSetBuild;
namespace Attrib { class DescSet; class EngineMan; }
namespace Strat { class LayerModel; class LaySeqAttribSet; }
namespace StratSynth { class DataMgr; }


/*!\brief Dialog specifying what to crossplot */

mExpClass(uiWellAttrib) uiStratSynthCrossplot : public uiDialog
{ mODTextTranslationClass(uiStratSynthCrossplot);
public:
				uiStratSynthCrossplot(uiParent*,
						    const StratSynth::DataMgr&);
				~uiStratSynthCrossplot();

    void			setRefLevel(const Strat::Level::ID&);

protected:

    const StratSynth::DataMgr* synthmgr_;
    const Strat::LayerModel&	lm_;
    ObjectSet<TypeSet<Interval<float> > > extrgates_;

    uiAttribDescSetBuild*	seisattrfld_	= nullptr;
    uiStratLaySeqAttribSetBuild* layseqattrfld_ = nullptr;
    uiStratSeisEvent*		evfld_		= nullptr;

    DataPointSet*		getData(const Attrib::DescSet&,
					const Strat::LaySeqAttribSet&,
					const Strat::Level&,
					const ZGate&,float zstep,
					const Strat::Level*);
    bool			extractSeisAttribs(DataPointSet&,
						   const Attrib::DescSet&);
    bool			extractLayerAttribs(DataPointSet&,
						const Strat::LaySeqAttribSet&,
						const Strat::Level*);
    bool			extractModelNr(DataPointSet&) const;
    void			fillPosFromZSampling(DataPointSet&,
						     const TimeDepthModel&,
						     const SeisTrcInfo&,
						     float zstep,float maxtwt,
						     const ZGate&);
    void			fillPosFromLayerSampling(DataPointSet&,
						     const TimeDepthModel&,
						     const SeisTrcInfo&,
						     const ZGate&,int iseq);
    void			launchCrossPlot(const DataPointSet&,
						const Strat::Level&,
						const Strat::Level*,
						const ZGate&,float zstep);
    Attrib::EngineMan*		createEngineMan(const Attrib::DescSet&) const;
    void			preparePreStackDescs();

    bool			handleUnsaved();
    bool			rejectOK(CallBacker*) override;
    bool			acceptOK(CallBacker*) override;

};



