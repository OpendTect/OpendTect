/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Apr 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellattribxplot.cc,v 1.37 2010-11-29 11:57:18 cvsnageswara Exp $";

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
#include "posfilterset.h"
#include "seisioobjinfo.h"
#include "wellextractdata.h"
#include "wellmarker.h"

#include "mousecursor.h"
#include "uidatapointset.h"
#include "uiposfilterset.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "unitofmeasure.h"


using namespace Attrib;

uiWellAttribCrossPlot::uiWellAttribCrossPlot( uiParent* p,
					      const Attrib::DescSet& d )
	: uiDialog(p,uiDialog::Setup("Attribute/Well cross-plotting",
		     "Select attributes and logs for cross-plot"
		     ,"111.1.1").modal(false))
	, ads_(*new Attrib::DescSet(d.is2D()))
    	, posfiltfld_(0)
    	, curdps_(0)
    	, dpsdispmgr_(0)
{
    uiLabeledListBox* llba = new uiLabeledListBox( this, "Attributes", true );
    attrsfld_ = llba->box();
    uiLabeledListBox* llbw = new uiLabeledListBox( this, "Wells", true );
    wellsfld_ = llbw->box();
    llbw->attach( alignedBelow, llba );
    uiLabeledListBox* llbl = new uiLabeledListBox( this, "Logs", true,
						   uiLabeledListBox::RightTop );
    logsfld_ = llbl->box();
    llbl->attach( rightTo, llbw );

    const float inldist = SI().inlDistance();
    radiusfld_ = new uiGenInput( this, "Radius around wells",
	    			 FloatInpSpec((float)((int)(inldist+.5))) );
    radiusfld_->attach( alignedBelow, llbw );
    radiusfld_->attach( ensureBelow, llbl );

    uiGroup* attgrp = radiusfld_;
    if ( !ads_.is2D() )
    {
	uiPosFilterSet::Setup fsu( false );
	fsu.seltxt( "Filter positions" ).incprovs( true );
	posfiltfld_ = new uiPosFilterSetSel( this, fsu );
	posfiltfld_->attach( alignedBelow, radiusfld_ );
	attgrp = posfiltfld_;
    }

    uiLabeledComboBox* llc0 = new uiLabeledComboBox( this, "Extract between" );
    topmarkfld_ = llc0->box();
    topmarkfld_->setName( "Top marker" );
    llc0->attach( alignedBelow, attgrp );
    botmarkfld_ = new uiComboBox( this, "Bottom marker" );
    botmarkfld_->attach( rightOf, llc0 );
    BufferString txt = "Distance above/below ";
    txt += SI().depthsInFeetByDefault() ? "(ft)" : "(m)";
    abovefld_ = new uiGenInput( this, txt,
	    			FloatInpSpec(0).setName("Distance above") );
    abovefld_->attach( alignedBelow, llc0 );
    belowfld_ = new uiGenInput( this, "", 
	    			FloatInpSpec(0).setName("Distance below") );
    belowfld_->attach( rightOf, abovefld_ );

    logresamplfld_ = new uiGenInput( this, "Log resampling method",
		  StringListInpSpec(Well::LogDataExtracter::SamplePolNames()) );
    logresamplfld_->attach( alignedBelow, abovefld_ );

    setDescSet( d );
    finaliseDone.notify( mCB(this,uiWellAttribCrossPlot,initWin) );
}

#define mDPM DPM(DataPackMgr::PointID())

uiWellAttribCrossPlot::~uiWellAttribCrossPlot()
{
    delete const_cast<Attrib::DescSet*>(&ads_);
    deepErase( wellobjs_ );
}


void uiWellAttribCrossPlot::initWin( CallBacker* )
{
    wellsfld_->setEmpty(); logsfld_->setEmpty();
    topmarkfld_->setEmpty(); botmarkfld_->setEmpty();
    deepErase( wellobjs_ );

    Well::InfoCollector wic;
    uiTaskRunner tr( this );
    if ( !tr.execute(wic) ) return;

    BufferStringSet markernms, lognms;
    markernms.add( Well::TrackSampler::sKeyDataStart() );
    for ( int iid=0; iid<wic.ids().size(); iid++ )
    {
	IOObj* ioobj = IOM().get( *wic.ids()[iid] );
	if ( !ioobj ) continue;
	wellobjs_ += ioobj;
	wellsfld_->addItem( ioobj->name() );

	const BufferStringSet& logs = *wic.logs()[iid];
	for ( int ilog=0; ilog<logs.size(); ilog++ )
	    lognms.addIfNew( logs.get(ilog) );
	const Well::MarkerSet& mrkrs = *wic.markers()[iid];
	for ( int imrk=0; imrk<mrkrs.size(); imrk++ )
	    markernms.addIfNew( mrkrs[imrk]->name() );
    }
    markernms.add( Well::TrackSampler::sKeyDataEnd() );

    for ( int idx=0; idx<lognms.size(); idx++ )
	logsfld_->addItem( lognms.get(idx) );
    topmarkfld_->addItems( markernms );
    botmarkfld_->addItems( markernms );
    topmarkfld_->setCurrentItem( 0 );
    botmarkfld_->setCurrentItem( markernms.size() - 1 );
}


const DataPointSet& uiWellAttribCrossPlot::getDPS() const
{ return *curdps_; }


void uiWellAttribCrossPlot::setDescSet( const Attrib::DescSet& newads )
{
    const_cast<Attrib::DescSet&>( ads_ ) = newads;
    adsChg();
}


void uiWellAttribCrossPlot::adsChg()
{
    attrsfld_->setEmpty();

    Attrib::SelInfo attrinf( &ads_, 0, ads_.is2D() );
    for ( int idx=0; idx<attrinf.attrnms_.size(); idx++ )
	attrsfld_->addItem( attrinf.attrnms_.get(idx), false );
    
    for ( int idx=0; idx<attrinf.ioobjids_.size(); idx++ )
    {
	BufferStringSet bss;
	SeisIOObjInfo sii( MultiID( attrinf.ioobjids_.get(idx) ) );
	sii.getDefKeys( bss, true );
	for ( int inm=0; inm<bss.size(); inm++ )
	{
	    const char* defkey = bss.get(inm).buf();
	    const char* ioobjnm = attrinf.ioobjnms_.get(idx).buf();
	    attrsfld_->addItem(
		    SeisIOObjInfo::defKey2DispName(defkey,ioobjnm) );
	}
    }
}


static void addDCDs( uiListBox* lb, ObjectSet<DataColDef>& dcds,
		     BufferStringSet& nms )
{
    for ( int idx=0; idx<lb->size(); idx++ )
    {
	if ( !lb->isSelected(idx) ) continue;
	const char* nm = lb->textOfItem(idx);
	nms.add( nm );
	dcds += new DataColDef( nm );
    }
}


#define mErrRet(s) { if ( s ) uiMSG().error(s); return false; }

bool uiWellAttribCrossPlot::extractWellData( const BufferStringSet& ioobjids,
					     const BufferStringSet& lognms,
					     ObjectSet<DataPointSet>& dpss )
{
    Well::TrackSampler wts( ioobjids, dpss, SI().zIsTime() );
    wts.for2d = false; wts.lognms = lognms;
    wts.locradius = radiusfld_->getfValue();
    wts.topmrkr = topmarkfld_->text(); wts.botmrkr = botmarkfld_->text();
    wts.above = abovefld_->getfValue(0,0);
    wts.below = belowfld_->getfValue(0,0);
    wts.mkdahcol = true;
    uiTaskRunner tr( this );
    if ( !tr.execute(wts) )
	return false;
    if ( dpss.isEmpty() )
	mErrRet("No wells found")

    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	Well::LogDataExtracter wlde( ioobjids, dpss, SI().zIsTime() );
	wlde.lognm_ = lognms.get(idx);
	wlde.samppol_ = (Well::LogDataExtracter::SamplePol)
	    				logresamplfld_->getIntValue();
	if ( !tr.execute(wlde) )
	    return false;
    }

    return true;
}


bool uiWellAttribCrossPlot::extractAttribData( DataPointSet& dps, int c1 )
{
    IOPar descsetpars;
    ads_.fillPar( descsetpars );
    const_cast<PosVecDataSet*>( &(dps.dataSet()) )->pars() = descsetpars;

    MouseCursorManager::setOverride( MouseCursor::Wait );
    Attrib::EngineMan aem; BufferString errmsg;
    PtrMan<Executor> tabextr = aem.getTableExtractor( dps, ads_, errmsg, c1 );
    MouseCursorManager::restoreOverride();
    if ( !errmsg.isEmpty() )
	mErrRet(errmsg)
    uiTaskRunner tr( this );
    return tr.execute( *tabextr );
}


#undef mErrRet
#define mErrRet(s) { deepErase(dcds); if ( s ) uiMSG().error(s); return false; }

bool uiWellAttribCrossPlot::acceptOK( CallBacker* )
{
    ObjectSet<DataColDef> dcds;
    BufferString unit( "MD" );
    SI().depthsInFeetByDefault() ? unit.add( "(ft)" ) : unit.add( "(m)" );
    dcds += new DataColDef( unit );
    BufferStringSet lognms; addDCDs( logsfld_, dcds, lognms );
    BufferStringSet attrnms; addDCDs( attrsfld_, dcds,  attrnms );
    if ( lognms.isEmpty() )
	mErrRet("Please select at least one log")

    BufferStringSet ioobjids, wellnms;
    for ( int idx=0; idx<wellsfld_->size(); idx++ )
    {
	if ( wellsfld_->isSelected(idx) )
	{
	    ioobjids.add( wellobjs_[idx]->key() );
	    wellnms.add( wellobjs_[idx]->name() );
	}
    }
    if ( ioobjids.isEmpty() )
	mErrRet("Please select at least one well")

    ObjectSet<DataPointSet> dpss;
    if ( !extractWellData(ioobjids,lognms,dpss) )
	mErrRet(0)

    PtrMan<Pos::Filter> filt = 0;
    if ( posfiltfld_ )
    {
	IOPar iop; posfiltfld_->fillPar( iop );
	filt = Pos::Filter::make( iop, false );
	if ( filt )
	{
	    uiTaskRunner tr( this );
	    if ( !filt->initialize(&tr) )
		return false;
	}
    }

    MouseCursorManager::setOverride( MouseCursor::Wait );
    DataPointSet* dps = new DataPointSet( TypeSet<DataPointSet::DataRow>(),
	    				  dcds, false, false );
    mDPM.addAndObtain( dps );

    const UnitOfMeasure* uom = UoMR().get( "Feet" );
    deepErase( dcds );
    const int nrattribs = attrnms.size();
    const int nrlogs = lognms.size() + 1;
    DataPointSet::DataRow dr;
    for ( int idps=0; idps<dpss.size(); idps++ )
    {
	DataPointSet& curdps = *dpss[idps];
	for ( int idr=0; idr<curdps.size(); idr++ )
	{
	    dr = curdps.dataRow( idr );
	    if ( filt && !filt->includes(dr.pos_.coord(),dr.pos_.z()) )
		continue;

	    DataPointSet::DataRow newdr( dr );
	    newdr.data_.setSize( nrattribs + nrlogs, mUdf(float) );
	    for ( int ilog=0; ilog<nrlogs; ilog++ )
		newdr.data_[ilog] = dr.data_[ilog];
	    for ( int iattr=0; iattr<nrattribs; iattr++ )
		newdr.data_[nrlogs+iattr] = mUdf(float);
	    newdr.setGroup( (unsigned short)(idps+1) );

	    if ( uom && !SI().zInFeet() && SI().depthsInFeetByDefault() )
		newdr.data_[0] = uom->getUserValueFromSI( newdr.data_[0] );

	    dps->setRow( newdr );
	}
    }

    deepErase( dpss );
    dps->dataChanged();
    MouseCursorManager::restoreOverride();
    if ( dps->isEmpty() )
	mErrRet("No positions found matching criteria")

    BufferString dpsnm( "Well data" );
    if ( !attrnms.isEmpty() )
    {
	dpsnm += " / Attributes";
	if ( !extractAttribData(*dps,nrlogs) )
	    return false;
    }

    dps->setName( dpsnm );
    uiDataPointSet* uidps = new uiDataPointSet( this,
	*dps, uiDataPointSet::Setup("Well attribute data",false), dpsdispmgr_ );
    uidps->setGroupType( "well" );
    uidps->setGroupNames( wellnms );
    return uidps->go() ? true : false;
}
