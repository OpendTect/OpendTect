/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratsynthcrossplot.cc,v 1.7 2011-01-25 12:56:24 cvsbert Exp $";

#include "uistratsynthcrossplot.h"
#include "uistratlayseqattrsetbuild.h"
#include "uiattribsetbuild.h"
#include "uidatapointset.h"
#include "uiseparator.h"
#include "uisplitter.h"
#include "uilabel.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "stratlevel.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "stratlayseqattrib.h"
#include "attribdescset.h"
#include "datapointset.h"
#include "posvecdataset.h"
#include "datacoldef.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "survinfo.h"
#include "valseriesevent.h"


uiStratSynthCrossplot::uiStratSynthCrossplot( uiParent* p, DataPack::ID dpid,
				    const Strat::LayerModel& lm )
    : uiDialog(p,Setup("Layer model/synthetics cross-plotting",
			mNoDlgTitle,mTODOHelpID))
    , packid_(dpid)
    , lm_(lm)
    , emptylbl_(0)
{
    if ( lm.isEmpty() )
	{ emptylbl_ = new uiLabel(this,"No input data"); return; }
    uiAttribDescSetBuild::Setup bsu( true );
    bsu.showdepthonlyattrs(false).showusingtrcpos(false).showps(false);
    seisattrfld_ = new uiAttribDescSetBuild( this, bsu );
    TypeSet<DataPack::FullID> fids;
    fids += DataPack::FullID( DataPackMgr::FlatID(), packid_ );
    seisattrfld_->setDataPackInp( fids );

    layseqattrfld_ = new uiStratLaySeqAttribSetBuild( this, lm_ );
    layseqattrfld_->attach( alignedWith, seisattrfld_ );

    uiSeparator* sep = new uiSeparator( this, "Separ" );
    sep->attach( stretchedBelow, layseqattrfld_ );

    uiGroup* extrposgrp = new uiGroup( this, "Extraction pos group" );
    uiLabeledComboBox* llvlfld = new uiLabeledComboBox( extrposgrp,
	    						"Reference level" );
    reflvlfld_ = llvlfld->box();
    extrposgrp->setHAlignObj( llvlfld );
    const Strat::LevelSet& lvls = Strat::LVLS();
    for ( int idx=0; idx<lvls.size(); idx++ )
	reflvlfld_->addItem( lvls.levels()[idx]->name() );

    BufferStringSet eventnms( VSEvent::TypeNames() ); eventnms.remove(0);
    snapfld_ = new uiGenInput( extrposgrp, "Snap to event",
	    			StringListInpSpec(eventnms) );
    snapfld_->setWithCheck( true );
    snapfld_->attach( alignedBelow, llvlfld );

    const float defstep = SI().zIsTime() ? SI().zStep() * 1000 : 4;
    extrwinfld_ = new uiGenInput( extrposgrp, "Extraction window",
	  FloatInpIntervalSpec(StepInterval<float>(0,0,defstep)) );
    extrwinfld_->attach( alignedBelow, snapfld_ );

    uiSplitter* spl = new uiSplitter( this, "Splitter", false );
    spl->addGroup( seisattrfld_ );
    spl->addGroup( layseqattrfld_ );
    spl->addGroup( extrposgrp );
}


uiStratSynthCrossplot::~uiStratSynthCrossplot()
{
}


#define mErrRet(s) { if ( s && *s ) uiMSG().error(s); return false; }

bool uiStratSynthCrossplot::acceptOK( CallBacker* )
{
    if ( emptylbl_ )
	return true;

    const Attrib::DescSet& seisattrs = seisattrfld_->descSet();
    if ( seisattrs.isEmpty() )
	mErrRet("Please define a seismic attribute")
    const Strat::LaySeqAttribSet& seqattrs = layseqattrfld_->attribSet();
    if ( seqattrs.isEmpty() )
	mErrRet("Please define a layer attribute")

    const Strat::Level* lvl = Strat::LVLS().get( reflvlfld_->text() );
    if ( !lvl )
	mErrRet("Cannot find selected stratigraphic level")

    StepInterval<float> extrwin( extrwinfld_->getFStepInterval() );
    if ( mIsUdf(extrwin.start) || mIsUdf(extrwin.stop) || mIsUdf(extrwin.step) )
	mErrRet("Please enter all extraction window parameters")

    extrwin.start *= 0.001; extrwin.stop *= 0.001; extrwin.step *= 0.001;
    DataPointSet* dps = getData( seisattrs, seqattrs, *lvl, extrwin );
    if ( !dps ) return false;
    bool rv = launchCrossPlot( *dps, *lvl, extrwin );
    delete dps; return rv;
}


DataPointSet* uiStratSynthCrossplot::getData( const Attrib::DescSet& seisattrs,
					const Strat::LaySeqAttribSet& seqattrs,
					const Strat::Level& lvl,
					const StepInterval<float>& extrwin )
{
    // DataPointSet* dps = seisattrs.getDataPointSet();
    DataPointSet* dps = 0;
    if ( !dps )
	{ uiMSG().error(seisattrs.errMsg()); return false; }
    for ( int iattr=0; iattr<seqattrs.size(); iattr++ )
	dps->dataSet().add(
		new DataColDef(seqattrs.attr(iattr).name(),toString(iattr,0)) );

    //TODO use attribute engine to fill dps cols
    //TODO use LaySeqAttribCalc to fill other dps cols
    return false;
}


bool uiStratSynthCrossplot::launchCrossPlot( const DataPointSet& dps,
					const Strat::Level& lvl,
					const StepInterval<float>& extrwin )
{
    BufferString wintitl( "Attributes at ", lvl.name() );
    wintitl.add( " [" ).add( extrwin.start )
	   .add( "-" ).add( extrwin.stop ).add( "]" );
    uiDataPointSet::Setup su( wintitl, false );
    uiDataPointSet* uidps = new uiDataPointSet( this, dps, su, 0 );
    uidps->setDeleteOnClose( true );
    uidps->show();
    return true;
}
