#ifndef uistratsynthcrossplot_h
#define uistratsynthcrossplot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
 RCS:           $Id: uistratsynthcrossplot.h,v 1.9 2011-01-27 15:57:33 cvshelene Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "datapack.h"

class uiLabel;
class SeisTrcBuf;
class uiGenInput;
class uiComboBox;
class DataPointSet;
class SeisTrcBufDataPack;
class uiAttribDescSetBuild;
class uiStratLaySeqAttribSetBuild;
namespace Strat { class Level; class LayerModel; class LaySeqAttribSet; }
namespace Attrib { class DescSet; class EngineMan; }


/*!\brief Dialog specifying what to crossplot */

mClass uiStratSynthCrossplot : public uiDialog
{
public:
				uiStratSynthCrossplot(uiParent*,
						const DataPack::FullID&,
						const Strat::LayerModel&);
				~uiStratSynthCrossplot();

protected:

    const Strat::LayerModel&	lm_;
    SeisTrcBufDataPack*		tbpack_;
    const DataPackMgr::ID	packmgrid_;

    uiAttribDescSetBuild*	seisattrfld_;
    uiStratLaySeqAttribSetBuild* layseqattrfld_;
    uiLabel*			emptylbl_;
    uiComboBox*			reflvlfld_;
    uiGenInput*			snapfld_;
    uiGenInput*			extrwinfld_;

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

    bool			acceptOK(CallBacker*);

    Attrib::EngineMan*		createEngineMan(const Attrib::DescSet&) const;

};



#endif
