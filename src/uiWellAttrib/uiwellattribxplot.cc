/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra / Bert
 Date:          March 2003 / Feb 2008
 RCS:           $Id: uiwellattribxplot.cc,v 1.1 2008-04-17 09:09:05 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiwellattribxplot.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "executor.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "survinfo.h"
#include "keystrs.h"
#include "posinfo.h"
#include "posvecdataset.h"
#include "seisioobjinfo.h"
#include "wellextractdata.h"
#include "wellmarker.h"

#include "mousecursor.h"
#include "uidatapointset.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "uitaskrunner.h"

using namespace Attrib;

uiWellAttribCrossPlot::uiWellAttribCrossPlot( uiParent* p,
					      const Attrib::DescSet& d )
	: uiDialog(p,uiDialog::Setup("Attribute/Well cross-plotting",
		     "Select attributes and logs for cross-plot"
		     ,"107.3.1").modal(false))
	, ads_(*new Attrib::DescSet(d.is2D()))
{
    uiLabeledListBox* llba = new uiLabeledListBox( this, "Attributes", true );
    attrsfld_ = llba->box();
    uiLabeledListBox* llbw = new uiLabeledListBox( this, "Wells", true );
    wellsfld_ = llbw->box();
    llbw->attach( alignedBelow, llba );
    uiLabeledListBox* llbl = new uiLabeledListBox( this, "Logs", true,
						   uiLabeledListBox::AboveMid );
    logsfld_ = llbl->box();
    llbl->attach( rightOf, llbw );

    radiusfld_ = new uiGenInput( this, "Radius around wells",
	    			 IntInpSpec((int)(SI().inlDistance()+.5)) );
    radiusfld_->attach( alignedBelow, llbl );

    uiLabeledComboBox* llc0 = new uiLabeledComboBox( this, "Extract between" );
    topmarkfld_ = llc0->box();
    llc0->attach( alignedBelow, radiusfld_ );
    botmarkfld_ = new uiComboBox( this, "Bottom marker" );
    botmarkfld_->attach( rightOf, llc0 );
    BufferString txt = "Distance above/below ";
    txt += SI().depthsInFeetByDefault() ? "(ft)" : "(m)";
    abovefld_ = new uiGenInput( this, txt, FloatInpSpec(0) );
    abovefld_->attach( alignedBelow, topmarkfld_ );
    belowfld_ = new uiGenInput( this, "", FloatInpSpec(0) );
    belowfld_->attach( rightOf, abovefld_ );

    logresamplfld_ = new uiGenInput( this, "Log resampling method",
		  StringListInpSpec(Well::LogDataExtracter::SamplePolNames) );
    logresamplfld_->attach( alignedBelow, llc0 );

    setDescSet( d );
    finaliseDone.notify( mCB(this,uiWellAttribCrossPlot,initWin) );
}


uiWellAttribCrossPlot::~uiWellAttribCrossPlot()
{
    delete const_cast<Attrib::DescSet*>(&ads_);
    deepErase( wellobjs_ );
}


void uiWellAttribCrossPlot::initWin( CallBacker* )
{
    Well::InfoCollector wic;
    uiTaskRunner tr( this );
    if ( !tr.execute(wic) ) return;

    BufferStringSet markernms, lognms;
    markernms.add( Well::TrackSampler::sKeyDataStart );
    for ( int iid=0; iid<wic.ids().size(); iid++ )
    {
	IOObj* ioobj = IOM().get( *wic.ids()[iid] );
	if ( !ioobj ) continue;
	wellobjs_ += ioobj;
	wellsfld_->addItem( ioobj->name() );

	const BufferStringSet& logs = *wic.logs()[iid];
	for ( int ilog=0; ilog<logs.size(); ilog++ )
	    lognms.addIfNew( logs.get(ilog) );
	const ObjectSet<Well::Marker>& mrkrs = *wic.markers()[iid];
	for ( int imrk=0; imrk<mrkrs.size(); imrk++ )
	    markernms.addIfNew( mrkrs[imrk]->name() );
    }
    markernms.add( Well::TrackSampler::sKeyDataEnd );

    logsfld_->addItems( lognms );
    topmarkfld_->addItems( markernms );
    botmarkfld_->addItems( markernms );
    topmarkfld_->setCurrentItem( 0 );
    botmarkfld_->setCurrentItem( markernms.size() - 1 );
}


void uiWellAttribCrossPlot::setDescSet( const Attrib::DescSet& newads )
{
    IOPar iop; newads.fillPar( iop );
    Attrib::DescSet& ads = const_cast<Attrib::DescSet&>( ads_ );
    ads.removeAll(); ads.usePar( iop );
    adsChg();
}


void uiWellAttribCrossPlot::adsChg()
{
    attrsfld_->empty();

    Attrib::SelInfo attrinf( &ads_, 0, ads_.is2D() );
    for ( int idx=0; idx<attrinf.attrnms.size(); idx++ )
	attrsfld_->addItem( attrinf.attrnms.get(idx), false );
    
    for ( int idx=0; idx<attrinf.ioobjids.size(); idx++ )
    {
	BufferStringSet bss;
	SeisIOObjInfo sii( MultiID( attrinf.ioobjids.get(idx) ) );
	sii.getDefKeys( bss, true );
	for ( int inm=0; inm<bss.size(); inm++ )
	{
	    const char* defkey = bss.get(inm).buf();
	    const char* ioobjnm = attrinf.ioobjnms.get(idx).buf();
	    attrsfld_->addItem(
		    SeisIOObjInfo::defKey2DispName(defkey,ioobjnm) );
	}
    }
    if ( !attrsfld_->isEmpty() )
	attrsfld_->setCurrentItem( int(0) );
}


#define mDPM DPM(DataPackMgr::PointID)
#define mErrRet(s) \
{ \
    if ( dps ) mDPM.release(dps->id()); \
    if ( s ) uiMSG().error(s); return false; \
}

bool uiWellAttribCrossPlot::acceptOK( CallBacker* )
{
    DataPointSet* dps = 0;

    ObjectSet<DataColDef> dcds;
    for ( int idx=0; idx<attrsfld_->size(); idx++ )
    {
	if ( attrsfld_->isSelected(idx) )
	    dcds += new DataColDef( attrsfld_->textOfItem(idx) );
    }
    if ( dcds.isEmpty() )
	mErrRet("Please select at least one attribute")
    for ( int idx=0; idx<logsfld_->size(); idx++ )
    {
	if ( logsfld_->isSelected(idx) )
	    dcds += new DataColDef( logsfld_->textOfItem(idx) );
    }

    dps = new DataPointSet( TypeSet<DataPointSet::DataRow>(), dcds,
	    		    ads_.is2D(), false );
    // Now extract ...!
    /*
    if ( dps->isEmpty() )
	mErrRet("No positions selected")
	*/

    dps->setName( "Attributes / Well data" );
    IOPar descsetpars;
    ads_.fillPar( descsetpars );
    const_cast<PosVecDataSet*>( &(dps->dataSet()) )->pars() = descsetpars;
    mDPM.add( dps );

    BufferString errmsg;
    Attrib::EngineMan aem;
    MouseCursorManager::setOverride( MouseCursor::Wait );
    PtrMan<Executor> tabextr = aem.getTableExtractor( *dps, ads_, errmsg );
    MouseCursorManager::restoreOverride();
    if ( !errmsg.isEmpty() ) mErrRet(errmsg)
    uiTaskRunner tr( this );
    if ( !tr.execute(*tabextr) )
	return false;

    uiDataPointSet* dlg = new uiDataPointSet( this, *dps,
			uiDataPointSet::Setup("Attribute data") );
    return dlg->go() ? true : false;
}
