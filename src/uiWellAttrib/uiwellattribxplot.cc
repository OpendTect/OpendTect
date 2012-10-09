/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Apr 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

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
#include "uimultiwelllogsel.h"
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

    welllogselfld_ = new uiMultiWellLogSel( this, 
	uiWellExtractParams::Setup().withsampling(true)
				    .withzintime(true)
				    .withzstep(true)
				    .withextractintime(true) );
    welllogselfld_->attach( ensureBelow, llba );

    const float inldist = SI().inlDistance();
    const char* distunit =  SI().getXYUnitString();
    BufferString radiusbuf( "  Radius around wells "); 
    radiusbuf += distunit;
    radiusfld_ = new uiGenInput( this, radiusbuf,
	    			 FloatInpSpec((float)((int)(inldist+.5))) );
    radiusfld_->attach( alignedBelow, llba );
    radiusfld_->attach( ensureBelow, welllogselfld_ );

    uiGroup* attgrp = radiusfld_;
    if ( !ads_.is2D() )
    {
	uiPosFilterSet::Setup fsu( false );
	fsu.seltxt( "Filter positions" ).incprovs( true );
	posfiltfld_ = new uiPosFilterSetSel( this, fsu );
	posfiltfld_->attach( alignedBelow, radiusfld_ );
	attgrp = posfiltfld_;
    }

    setDescSet( d );
}

#define mDPM DPM(DataPackMgr::PointID())

uiWellAttribCrossPlot::~uiWellAttribCrossPlot()
{
    delete const_cast<Attrib::DescSet*>(&ads_);
    deepErase( dpsset_ );
}


void uiWellAttribCrossPlot::initWin( CallBacker* )
{
    welllogselfld_->update();
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
	    attrsfld_->addItem( attrinf.is2D(defkey)
		    ? SeisIOObjInfo::defKey2DispName(defkey,ioobjnm)
		    : SeisIOObjInfo::def3DDispName(defkey,ioobjnm) );
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
    wts.for2d_ = false; wts.lognms_ = lognms;
    wts.locradius_ = radiusfld_->getfValue();
    wts.mkdahcol_ = true;
    wts.params_ = welllogselfld_->params();
    uiTaskRunner tr( this );
    if ( !tr.execute(wts) )
	return false;
    if ( dpss.isEmpty() )
	mErrRet("No wells found")
    bool founddata = false;
    for ( int idx=0; idx<dpss.size(); idx++ )
    {
	if ( !dpss[idx]->isEmpty() )
	    { founddata = true; break; }
    }
    if ( !founddata )
    {
	if ( dpss.size() == 1 )
	    mErrRet("No valid data points found in the well\n"
		    "(check ranges, undefined sections ....)")
	else
	    mErrRet("No valid data points found in any of the wells\n"
		    "(check ranges, undefined sections ....)")
    }

    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	Well::LogDataExtracter wlde( ioobjids, dpss, SI().zIsTime() );
	wlde.lognm_ = lognms.get(idx);
	wlde.samppol_ = welllogselfld_->params().samppol_; 
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
    BufferStringSet lognms; welllogselfld_->getSelLogNames( lognms );
    for ( int idx=0; idx<lognms.size(); idx++ )
	dcds += new DataColDef( lognms[idx]->buf() );
    if ( lognms.isEmpty() )
	mErrRet("Please select at least one log")
    BufferStringSet attrnms; addDCDs( attrsfld_, dcds,  attrnms );

    BufferStringSet ioobjids, wellnms;
    welllogselfld_->getSelWellNames( wellnms );
    welllogselfld_->getSelWellIDs( ioobjids );
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
    dpsset_ += uidps;
    uidps->setGroupType( "well" );
    uidps->setGroupNames( wellnms );
    return uidps->go() ? true : false;
}
