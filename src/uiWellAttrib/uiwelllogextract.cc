/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwelllogextract.h"

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
#include "welldata.h"
#include "wellextractdata.h"
#include "welllog.h"
#include "wellman.h"
#include "wellmarker.h"

#include "mousecursor.h"
#include "uiposfilterset.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "uimultiwelllogsel.h"
#include "uitaskrunner.h"
#include "unitofmeasure.h"


using namespace Attrib;

// uiWellLogExtractGrp::uiWellLogExtractGrp

uiWellLogExtractGrp::Setup::Setup( bool wa, bool singlog, const char* prop )
    : singlelog_(singlog)
    , withattrib_(wa)
    , prefpropnm_(prop)
{
}


uiWellLogExtractGrp::Setup::~Setup()
{
}


// uiWellLogExtractGrp

uiWellLogExtractGrp::uiWellLogExtractGrp( uiParent* p,
	const uiWellLogExtractGrp::Setup& setup, const Attrib::DescSet* d )
    : uiGroup(p)
    , ads_( d )
    , setup_(setup)
{
    welllogselfld_ =
	new uiMultiWellLogSel( this, uiWellExtractParams::Setup()
					.withsampling(true).withzstep(true)
					.withextractintime(SI().zIsTime())
					.singlelog(setup.singlelog_)
					.prefpropnm(setup.prefpropnm_));

    uiListBox::Setup asu( OD::ChooseZeroOrMore, uiStrings::sAttribute(mPlural));
    asu.lblpos( uiListBox::AboveMid );
    attrsfld_ = new uiListBox( this, asu );
    attrsfld_->display( setup.withattrib_, true );
    welllogselfld_->attach( ensureBelow, attrsfld_ );

    const float inldist = SI().inlDistance();
    uiString radiusbuf =  tr("Radius around wells %1")
						.arg(SI().getUiXYUnitString());
    radiusfld_ = new uiGenInput( this, radiusbuf,
				 FloatInpSpec((float)((int)(inldist+.5))) );
    radiusfld_->attach( alignedBelow, welllogselfld_ );

    if ( ads_ && !ads_->is2D() )
    {
	uiPosFilterSet::Setup fsu( false );
	fsu.seltxt( mJoinUiStrs(sFilter(),sPosition(mPlural).toLower()) )
							     .incprovs( true );
	posfiltfld_ = new uiPosFilterSetSel( this, fsu );
	posfiltfld_->attach( alignedBelow, radiusfld_ );
    }

    setDescSet( d );
}

#define mDPM DPM(DataPackMgr::PointID())

uiWellLogExtractGrp::~uiWellLogExtractGrp()
{
    releaseDPS();
}


void uiWellLogExtractGrp::releaseDPS()
{
    curdps_ = nullptr;
}


const DataPointSet* uiWellLogExtractGrp::getDPS() const
{ return curdps_; }


void uiWellLogExtractGrp::setDescSet( const Attrib::DescSet* newads )
{
    ads_  = newads;
    adsChg();
    welllogselfld_->update();
}


void uiWellLogExtractGrp::adsChg()
{
    if ( !ads_ ) return;

    attrsfld_->setEmpty();

    Attrib::SelInfo attrinf( ads_, 0, ads_->is2D() );
    for ( int idx=0; idx<attrinf.attrnms_.size(); idx++ )
    {
	const Attrib::Desc* desc = ads_->getDesc( attrinf.attrids_[idx] );
	if ( desc && desc->is2D() && desc->isPS() )
	{
	    attrinf.attrnms_.removeSingle( idx );
	    attrinf.attrids_.removeSingle( idx );
	    idx--;
	    continue;
	}
	attrsfld_->addItem( toUiString(attrinf.attrnms_.get(idx)), false );
	attrsfld_->setChosen( attrsfld_->size()-1, true );
    }

    for ( int idx=0; idx<attrinf.ioobjids_.size(); idx++ )
    {
	SeisIOObjInfo sii( MultiID( attrinf.ioobjids_.get(idx) ) );
	if ( sii.isPS() && sii.is2D() )
	{
	    attrinf.ioobjids_.removeSingle( idx );
	    attrinf.ioobjnms_.removeSingle( idx );
	    idx--;
	    continue;
	}

	const char* ioobjnm = attrinf.ioobjnms_.get(idx).buf();
	attrsfld_->addItem( toUiString("[%1]").arg(ioobjnm) );
    }
}


static void addDCDs( uiListBox* lb, ObjectSet<DataColDef>& dcds,
		     BufferStringSet& nms )
{
    for ( int idx=0; idx<lb->size(); idx++ )
    {
	if ( !lb->isChosen(idx) )
	    continue;
	const char* nm = lb->textOfItem(idx);
	nms.add( nm );
	dcds += new DataColDef( nm );
    }
}



#define mErrRet(s) { if ( !s.isEmpty() ) uiMSG().error(s); return false; }

bool uiWellLogExtractGrp::extractWellData( const TypeSet<MultiID>& ioobjids,
					     const BufferStringSet& lognms,
					     ObjectSet<DataPointSet>& dpss )
{
    Well::TrackSampler wts( ioobjids, dpss, SI().zIsTime() );
    wts.for2d_ = false; wts.lognms_ = lognms;
    wts.locradius_ = !radiusfld_ ? 0.f : radiusfld_->getFValue();
    wts.mkdahcol_ = true;
    wts.params_ = welllogselfld_->params();
    if ( !wts.params_.isInTime() )
    {
	wts.params_.zstep_ = getConvertedValue( wts.params_.zstep_,
				UnitOfMeasure::surveyDefDepthUnit(),
				UnitOfMeasure::surveyDefDepthStorageUnit() );
	wts.params_.setTopMarker( wts.params_.topMarker(),
			      getConvertedValue(wts.params_.topOffset(),
				UnitOfMeasure::surveyDefDepthUnit(),
				UnitOfMeasure::surveyDefDepthStorageUnit()) );
	wts.params_.setBotMarker( wts.params_.botMarker(),
			      getConvertedValue(wts.params_.botOffset(),
				UnitOfMeasure::surveyDefDepthUnit(),
				UnitOfMeasure::surveyDefDepthStorageUnit()) );
	if ( !wts.params_.getFixedRange().isUdf() )
	{
	    float start = wts.params_.getFixedRange().start;
	    float stop = wts.params_.getFixedRange().stop;
	    start = getConvertedValue( start,
				       UnitOfMeasure::surveyDefDepthUnit(),
				  UnitOfMeasure::surveyDefDepthStorageUnit() );
	    stop = getConvertedValue( stop, UnitOfMeasure::surveyDefDepthUnit(),
				  UnitOfMeasure::surveyDefDepthStorageUnit() );
	    wts.params_.setFixedRange( Interval<float>(start, stop), false );
	}
    }
    uiTaskRunner taskrunner( this );
    if ( !TaskRunner::execute( &taskrunner, wts ) )
	return false;
    if ( dpss.isEmpty() )
	mErrRet(tr("No wells found"))
    bool founddata = false;
    for ( int idx=0; idx<dpss.size(); idx++ )
    {
	if ( !dpss[idx]->isEmpty() )
	    { founddata = true; break; }
    }
    if ( !founddata )
    {
	if ( dpss.size() == 1 )
	    mErrRet(tr("No valid data points found in the well\n"
		    "(check ranges, undefined sections ....)"))
	else
	    mErrRet(tr("No valid data points found in any of the wells\n"
		    "(check ranges, undefined sections ....)"))
    }

    ExecutorGroup execgrp( "Reading log data" );
    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	auto* wlde = new Well::LogDataExtracter(ioobjids, dpss, SI().zIsTime());
	wlde->lognm_ = lognms.get(idx);
	wlde->samppol_ = welllogselfld_->params().samppol_;
	execgrp.add( wlde );
    }

    const bool res = TaskRunner::execute( &taskrunner, execgrp );
    return res;
}


bool uiWellLogExtractGrp::extractAttribData( DataPointSet& dps, int c1 )
{
    IOPar descsetpars;
    ads_->fillPar( descsetpars );
    const_cast<PosVecDataSet*>( &(dps.dataSet()) )->pars() = descsetpars;

    MouseCursorManager::setOverride( MouseCursor::Wait );
    Attrib::EngineMan aem;
    uiString errmsg;
    PtrMan<Executor> tabextr = aem.getTableExtractor( dps, *ads_, errmsg, c1 );
    MouseCursorManager::restoreOverride();
    if ( !errmsg.isEmpty() )
	mErrRet(errmsg)

    uiTaskRunner taskrunner( this );
    const bool res = TaskRunner::execute( &taskrunner, *tabextr );
    return res;
}


void uiWellLogExtractGrp::getWellNames( BufferStringSet& wellnms )
{ welllogselfld_->getSelWellNames( wellnms ); }

void uiWellLogExtractGrp::getSelLogNames( BufferStringSet& lognms )
{ welllogselfld_->getSelLogNames( lognms ); }

#undef mErrRet
#define mErrRet(s) \
{ deepErase(dcds); if ( !s.isEmpty() ) uiMSG().error(s); return false; }

bool uiWellLogExtractGrp::extractDPS()
{
    ObjectSet<DataColDef> dcds;

    BufferStringSet wellnms;
    getWellNames( wellnms );

    TypeSet<MultiID> ioobjids;
    welllogselfld_->getSelWellIDs( ioobjids );
    if ( ioobjids.isEmpty() )
	mErrRet(uiStrings::phrSelect(tr("at least one well")))

    BufferStringSet lognms;
    getSelLogNames( lognms );
    if ( lognms.isEmpty() )
	mErrRet(uiStrings::phrSelect(tr("at least one log")))

    dcds += new DataColDef( sKey::MD(), nullptr,
			    UnitOfMeasure::surveyDefDepthUnit() );

    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	ConstRefMan<Well::Data> wd = Well::MGR().get( ioobjids.get(0),
						      Well::LogInfos );
	const char* lognm = lognms.get( idx ).buf();
	const Well::Log* log = wd ? wd->getLog( lognm ) : nullptr;
	if ( !log ) // should not happen
	    continue;

	dcds += new DataColDef( lognm, nullptr, log->unitOfMeasure() );
    }

    BufferStringSet attrnms;
    if ( ads_ )
	addDCDs( attrsfld_, dcds,  attrnms );

    ObjectSet<DataPointSet> dpss;
    if ( !extractWellData(ioobjids,lognms,dpss) )
	mErrRet(uiStrings::sEmptyString())

    for ( const auto* dps : dpss )
    {
	bool badunits = false;
	for ( int icol=0; icol<dps->nrCols(); icol++ )
	{
	    if ( icol!=0 && dcds.get(icol)->unit_ != dps->colDef( icol ).unit_ )
	    {
		uiMSG().warning( tr("Inconsistent units found\n"
	 "All data will be converted to the same units as the first well.") );
		badunits = true;
		break;
	    }
	}
	if ( badunits )
	    break;
    }

    PtrMan<Pos::Filter> filt = 0;
    if ( posfiltfld_ )
    {
	IOPar iop; posfiltfld_->fillPar( iop );
	filt = Pos::Filter::make( iop, false );
	if ( filt )
	{
	    uiTaskRunner taskrunner( this );
	    if ( !filt->initialize(&taskrunner) )
		return false;
	}
    }

    MouseCursorManager::setOverride( MouseCursor::Wait );
    if ( curdps_ )
	releaseDPS();
    curdps_ =
	new DataPointSet( TypeSet<DataPointSet::DataRow>(), dcds, false, false);
    mDPM.add( curdps_ );
    curdps_->ref();

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
	    {
		const UnitOfMeasure* out_uom = curdps_->colDef( ilog ).unit_;
		const UnitOfMeasure* in_uom = curdps.colDef( ilog ).unit_;
		newdr.data_[ilog] = getConvertedValue( dr.data_[ilog], in_uom,
						       out_uom );
	    }
	    for ( int iattr=0; iattr<nrattribs; iattr++ )
		newdr.data_[nrlogs+iattr] = mUdf(float);
	    newdr.setGroup( (unsigned short)(idps+1) );

	    curdps_->setRow( newdr );
	}
    }

    deepErase( dpss );
    curdps_->dataChanged();
    MouseCursorManager::restoreOverride();
    if ( curdps_->isEmpty() )
	mErrRet(uiStrings::phrCannotFind(uiStrings::sPosition(2).toLower()))

    BufferString dpsnm( "Well data: " );
    for ( int idx=0; idx<wellnms.size(); idx++ )
    {
	dpsnm += wellnms[idx]->buf();
	if ( idx!=wellnms.size()-1 )
	    dpsnm += ",";
    }

    if ( !attrnms.isEmpty() )
    {
	dpsnm += " / Attributes";
	if ( !extractAttribData(*curdps_,nrlogs) )
	    return false;
    }

    curdps_->setName( dpsnm );
    return true;
}
