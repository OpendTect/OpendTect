#ifndef uistratsynthcrossplot_h
#define uistratsynthcrossplot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
 RCS:           $Id: uistratsynthcrossplot.h,v 1.16 2011-06-08 14:19:09 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "datapack.h"

class uiLabel;
class SeisTrcBuf;
class TimeDepthModel;
class uiGenInput;
class uiComboBox;
class DataPointSet;
class uiStratSeisEvent;
class SeisTrcBufDataPack;
class uiAttribDescSetBuild;
class uiStratLaySeqAttribSetBuild;
namespace Strat { class Level; class LayerModel; class LaySeqAttribSet; }
namespace Attrib { class DescSet; class EngineMan; }
namespace PreStack { class GatherSetDataPack; }


/*!\brief Dialog specifying what to crossplot */

mClass uiStratSynthCrossplot : public uiDialog
{
public:
				uiStratSynthCrossplot(uiParent*,
					const DataPack::FullID& poststack,
					const Strat::LayerModel&,
					const ObjectSet<const TimeDepthModel>&,
					const DataPack::FullID& prestackid=-1);
				~uiStratSynthCrossplot();

    void			setRefLevel(const char*);

protected:

    const Strat::LayerModel&	lm_;
    SeisTrcBufDataPack*		tbpack_;
    PreStack::GatherSetDataPack* pspack_;
    const DataPackMgr::ID	packmgrid_;
    const DataPackMgr::ID	pspackmgrid_;
    const ObjectSet<const TimeDepthModel>& d2tmodels_;

    uiAttribDescSetBuild*	seisattrfld_;
    uiStratLaySeqAttribSetBuild* layseqattrfld_;
    uiStratSeisEvent*		evfld_;
    uiLabel*			emptylbl_;

    DataPointSet*		getData(const Attrib::DescSet&,
	    				const Strat::LaySeqAttribSet&,
					const Strat::Level&,
					const StepInterval<float>&);
    bool			extractSeisAttribs(DataPointSet&,
	    					   const Attrib::DescSet&);
    bool			extractLayerAttribs(DataPointSet&,
						const Strat::LaySeqAttribSet&);
    bool			launchCrossPlot(const DataPointSet&,
					const Strat::Level&,
					const StepInterval<float>&);
    Attrib::EngineMan*		createEngineMan(const Attrib::DescSet&) const;

    bool			acceptOK(CallBacker*);

};



#endif
