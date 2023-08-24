#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellattribmod.h"

#include "uidialog.h"
#include "uistring.h"

#include "datapointset.h"
#include "stratlevel.h"

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

    void			setRefLevel(const Strat::LevelID&);

protected:

    const StratSynth::DataMgr* synthmgr_;
    const Strat::LayerModel&	lm_;
    ObjectSet<TypeSet<Interval<float> > > extrgates_;

    uiAttribDescSetBuild*	seisattrfld_	= nullptr;
    uiStratLaySeqAttribSetBuild* layseqattrfld_ = nullptr;
    uiStratSeisEvent*		evfld_		= nullptr;

    RefMan<DataPointSet>	getData(const Attrib::DescSet&,
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
						     bool hasintegrated,
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
