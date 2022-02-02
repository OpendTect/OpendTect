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
#include "uistring.h"

class SeisTrcInfo;
class SeisTrcBuf;
class TimeDepthModel;
class DataPointSet;
class SyntheticData;
class uiStratSeisEvent;
class SeisTrcBufDataPack;
class uiAttribDescSetBuild;
class uiStratLaySeqAttribSetBuild;
namespace Strat { class Level; class LayerModel; class LaySeqAttribSet; }
namespace Attrib { class DescSet; class EngineMan; }
namespace PreStack { class GatherSetDataPack; }


/*!\brief Dialog specifying what to crossplot */

mExpClass(uiWellAttrib) uiStratSynthCrossplot : public uiDialog
{ mODTextTranslationClass(uiStratSynthCrossplot);
public:
				uiStratSynthCrossplot(uiParent*,
				    const Strat::LayerModel&,
				    const ObjectSet<const SyntheticData>&);
				~uiStratSynthCrossplot();

    void			setRefLevel(const char*);
    uiString			errMsg() const
				{ return errmsg_; }

protected:

    const Strat::LayerModel&	lm_;

    const ObjectSet<const SyntheticData>& synthdatas_;
    ObjectSet<TypeSet<Interval<float> > > extrgates_;

    uiAttribDescSetBuild*	seisattrfld_;
    uiStratLaySeqAttribSetBuild* layseqattrfld_;
    uiStratSeisEvent*		evfld_;

    uiString			errmsg_;

    DataPointSet*		getData(const Attrib::DescSet&,
					const Strat::LaySeqAttribSet&,
					const Strat::Level&,
					const Interval<float>&, float zstep,
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
						     const Interval<float>&);
    void			fillPosFromLayerSampling(DataPointSet&,
						     const TimeDepthModel&,
						     const SeisTrcInfo&,
						     const Interval<float>&,
						     int iseq);
    void			launchCrossPlot(const DataPointSet&,
						const Strat::Level&,
						const Strat::Level*,
						const Interval<float>&,
						float zstep);
    Attrib::EngineMan*		createEngineMan(const Attrib::DescSet&) const;
    void			preparePreStackDescs();

    bool			handleUnsaved();
    bool			rejectOK(CallBacker*);
    bool			acceptOK(CallBacker*);

};



