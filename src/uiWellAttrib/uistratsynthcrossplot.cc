/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratsynthcrossplot.cc,v 1.4 2011-01-13 14:52:13 cvsbert Exp $";

#include "uistratsynthcrossplot.h"
#include "uistratlayseqattrsetbuild.h"
#include "uiattribsetbuild.h"
#include "uisplitter.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "seisbuf.h"
#include "seistrc.h"


#include "uilabel.h"

uiStratSynthCrossplot::uiStratSynthCrossplot( uiParent* p, DataPack::ID dpid,
				    const Strat::LayerModel& lm )
    : uiDialog(p,Setup("Layer model/synthetics cross-plotting",
			mNoDlgTitle,mTODOHelpID))
    , packid_(dpid)
    , lm_(lm)
{
    uiAttribDescSetBuild::Setup bsu( true );
    bsu.showdepthonlyattrs(false).showusingtrcpos(false).showps(false);
    seisattrfld_ = new uiAttribDescSetBuild( this, bsu );
    TypeSet<DataPack::FullID> fids;
    fids += DataPack::FullID( DataPackMgr::FlatID(), packid_ );
    seisattrfld_->setDataPackInp( fids );

    layseqattrfld_ = new uiStratLaySeqAttribSetBuild( this, lm_ );

    uiSplitter* spl = new uiSplitter( this, "Splitter", false );
    spl->addGroup( seisattrfld_ );
    spl->addGroup( layseqattrfld_ );
}


uiStratSynthCrossplot::~uiStratSynthCrossplot()
{
}


bool uiStratSynthCrossplot::acceptOK( CallBacker* )
{
    return true;
}
