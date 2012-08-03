#ifndef uistratsynthcrossplot_h
#define uistratsynthcrossplot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
 RCS:           $Id: uistratsynthcrossplot.h,v 1.21 2012-08-03 13:01:21 cvskris Exp $
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
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

mClass(uiWellAttrib) uiStratSynthCrossplot : public uiDialog
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

    bool			acceptOK(CallBacker*);

};



#endif

