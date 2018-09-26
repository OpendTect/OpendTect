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
class SeisTrcInfo;
class TimeDepthModel;
class uiStratSeisEvent;
class uiAttribDescSetBuild;
class uiStratLaySeqAttribSetBuild;
namespace Attrib { class DescSet; class EngineMan; }
namespace Strat { class LayerModel; class LaySeqAttribSet; }
namespace StratSynth { class DataMgr; }
namespace SynthSeis { class DataSet; }


/*!\brief Dialog specifying what to crossplot */

mExpClass(uiWellAttrib) uiStratSynthCrossplot : public uiDialog
{ mODTextTranslationClass(uiStratSynthCrossplot);
public:

    typedef StratSynth::DataMgr	DataMgr;

				uiStratSynthCrossplot(uiParent*,const DataMgr&);
				~uiStratSynthCrossplot();

    void			setRefLevel(const Strat::Level::ID&);

protected:

    typedef TypeSet<ZGate>	ExtrGateSet;
    typedef Attrib::DescSet	DescSet;
    typedef Strat::Level	Level;
    typedef Strat::LaySeqAttribSet LaySeqAttribSet;
    typedef Strat::LayerModel	LayerModel;

    ConstRefMan<DataMgr>	synthmgr_;
    const LayerModel&		lm_;
    ObjectSet<ExtrGateSet>	extrgates_;

    uiAttribDescSetBuild*	seisattrfld_		= 0;
    uiStratLaySeqAttribSetBuild* layseqattrfld_		= 0;
    uiStratSeisEvent*		evfld_			= 0;

    DataPointSet*		getData(const DescSet&,
					const LaySeqAttribSet&,const Level&,
					const ZGate&,float zstep,const Level*);
    bool			extractSeisAttribs(DataPointSet&,
						   const DescSet&);
    bool			extractLayerAttribs(DataPointSet&,
					const LaySeqAttribSet&,const Level*);
    bool			extractModelNr(DataPointSet&) const;
    void			fillPosFromZSampling(DataPointSet&,
					const TimeDepthModel&,
					const SeisTrcInfo&,float zstep,
					float maxtwt,const ZGate&);
    void			fillPosFromLayerSampling(DataPointSet&,
					const TimeDepthModel&,
					const SeisTrcInfo&,const ZGate&,int);
    void			launchCrossPlot(const DataPointSet&,
					const Level&,const Level*,
					const ZGate&,float zstep);
    Attrib::EngineMan*		createEngineMan(const DescSet&) const;
    void			preparePreStackDescs();

    bool			handleUnsaved();
    bool			rejectOK();
    bool			acceptOK();

};
