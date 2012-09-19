#ifndef uistratsynthcrossplot_h
#define uistratsynthcrossplot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
 RCS:           $Id: uistratsynthcrossplot.h,v 1.20 2011/10/05 12:25:32 cvsbruno Exp $
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
class SyntheticData;
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
				    const Strat::LayerModel&,
				    const ObjectSet<SyntheticData>&);

    void			setRefLevel(const char*);
    const char*			errMsg() const 
    				{ return errmsg_.isEmpty() ? 0 : errmsg_.buf();}

protected:

    const Strat::LayerModel&	lm_;

    const ObjectSet<SyntheticData>& synthdatas_;

    uiAttribDescSetBuild*	seisattrfld_;
    uiStratLaySeqAttribSetBuild* layseqattrfld_;
    uiStratSeisEvent*		evfld_;

    BufferString		errmsg_;

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

    bool			handleUnsaved();
    bool			rejectOK(CallBacker*);
    bool			acceptOK(CallBacker*);

};



#endif
