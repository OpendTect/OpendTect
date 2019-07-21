/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          July 2012
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
#include "ioobj.h"
#include "iopar.h"
#include "survinfo.h"
#include "posinfo.h"
#include "posvecdataset.h"
#include "posfilterset.h"
#include "seisioobjinfo.h"
#include "wellextractdata.h"
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

uiWellLogExtractGrp::uiWellLogExtractGrp( uiParent* p,
	const uiWellLogExtractGrp::Setup& setup, const Attrib::DescSet* d )
	: uiGroup(p)
	, ads_( d )
	, attrsfld_(0)
	, posfiltfld_(0)
	, radiusfld_(0)
	, curdps_(0)
	, setup_(setup)
{
    welllogselfld_ =
	new uiMultiWellLogSel( this, setup.singlelog_,
					&uiWellExtractParams::Setup()
					.withsampling(true).withzstep(true)
					.withextractintime(SI().zIsTime())
					.prefpropnm(setup.prefpropnm_));

    uiListBox::Setup asu( OD::ChooseZeroOrMore, uiStrings::sAttribute(mPlural));
    asu.lblpos( uiListBox::AboveMid );
    attrsfld_ = new uiListBox( this, asu );
    attrsfld_->display( setup.withattrib_, true );
    welllogselfld_->attach( ensureBelow, attrsfld_ );

    const float inldist = SI().inlDistance();
    uiString radiusstr = tr("Radius around wells").withSurvXYUnit();
    radiusfld_ = new uiGenInput( this, radiusstr,
				 FloatInpSpec((float)((int)(inldist+.5))) );
    if ( attrsfld_ )
	radiusfld_->attach( alignedBelow, attrsfld_ );
    else
	radiusfld_->attach( alignedBelow, welllogselfld_ );
    radiusfld_->attach( ensureBelow, welllogselfld_ );

    if ( ads_ && !ads_->is2D() )
    {
	uiPosFilterSet::Setup fsu( false );
	fsu.seltxt( tr("Filter positions") ).incprovs( true );
	posfiltfld_ = new uiPosFilterSetSel( this, fsu );
	posfiltfld_->attach( alignedBelow, radiusfld_ );
    }

    setDescSet( d );
}

#define mDPM DPM(DataPackMgr::PointID())

uiWellLogExtractGrp::~uiWellLogExtractGrp()
{}


void uiWellLogExtractGrp::releaseDPS()
{
    curdps_ = 0;
}


const DataPointSet* uiWellLogExtractGrp::getDPS() const
{
    return curdps_;
}


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

    Attrib::SelInfo attrinf( *ads_ );
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
	SeisIOObjInfo sii( DBKey( attrinf.ioobjids_.get(idx) ) );
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
	const char* nm = lb->itemText(idx);
	nms.add( nm );
	dcds += new DataColDef( nm );
    }
}



#define mErrRet(s) { if ( !s.isEmpty() ) uiMSG().error(s); return false; }

bool uiWellLogExtractGrp::extractWellData( const DBKeySet& ioobjids,
					     const BufferStringSet& lognms,
					     ObjectSet<DataPointSet>& dpss )
{
    Well::TrackSampler wts( ioobjids, dpss, SI().zIsTime() );
    wts.for2d_ = false; wts.lognms_ = lognms;
    wts.locradius_ = !radiusfld_ ? 0.f : radiusfld_->getFValue();
    wts.mkdahcol_ = true;
    wts.params_ = *welllogselfld_->getWellExtractParams();
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

    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	Well::LogDataExtracter wlde( ioobjids, dpss, SI().zIsTime() );
	wlde.lognm_ = lognms.get(idx);
	wlde.samppol_ = welllogselfld_->getWellExtractParams()->samppol_;
	if ( !TaskRunner::execute( &taskrunner, wlde ) )
	    return false;
    }

    return true;
}


bool uiWellLogExtractGrp::extractAttribData( DataPointSet& dps, int c1 )
{
    IOPar descsetpars;
    ads_->fillPar( descsetpars );
    const_cast<PosVecDataSet*>( &(dps.dataSet()) )->pars() = descsetpars;

    MouseCursorManager::setOverride( MouseCursor::Wait );
    Attrib::EngineMan aem; uiRetVal uirv;
    PtrMan<Executor> tabextr = aem.getTableExtractor( dps, *ads_, uirv, c1 );
    MouseCursorManager::restoreOverride();
    if ( !uirv.isOK() )
	mErrRet(uirv)
    uiTaskRunner taskrunner( this );
    return TaskRunner::execute( &taskrunner, *tabextr );
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

    BufferString unit( "MD (" );
    unit.add( toString(UnitOfMeasure::zUnitAnnot(false,true)) ).add( ")" );
    dcds += new DataColDef( unit );
    BufferStringSet lognms; welllogselfld_->getSelLogNames( lognms );
    for ( int idx=0; idx<lognms.size(); idx++ )
	dcds += new DataColDef( lognms[idx]->buf() );
    if ( lognms.isEmpty() )
	mErrRet(uiStrings::phrPlsSelectAtLeastOne(uiStrings::sLog()))
    BufferStringSet attrnms;
    if ( ads_ )
	addDCDs( attrsfld_, dcds,  attrnms );

    DBKeySet ioobjids; BufferStringSet wellnms;
    welllogselfld_->getSelWellNames( wellnms );
    welllogselfld_->getSelWellIDs( ioobjids );
    if ( ioobjids.isEmpty() )
	mErrRet(uiStrings::phrPlsSelectAtLeastOne(uiStrings::sWell()))

    ObjectSet<DataPointSet> dpss;
    if ( !extractWellData(ioobjids,lognms,dpss) )
	mErrRet(uiString::empty())

    PtrMan<Pos::Filter> filt = 0;
    if ( posfiltfld_ )
    {
	IOPar iop; posfiltfld_->fillPar( iop );
	filt = Pos::Filter::make( iop, false );
	if ( filt && !filt->initialize(uiTaskRunnerProvider(this)) )
	    return false;
    }

    MouseCursorManager::setOverride( MouseCursor::Wait );
    curdps_ =
	new DataPointSet( TypeSet<DataPointSet::DataRow>(), dcds, false, false);
    mDPM.add( curdps_ );

    const UnitOfMeasure* uom = UnitOfMeasure::feetUnit();
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

	    if ( uom && !SI().zInFeet() && SI().depthsInFeet() )
		newdr.data_[0] = uom->getUserValueFromSI( newdr.data_[0] );

	    curdps_->setRow( newdr );
	}
    }

    deepUnRef( dpss );
    curdps_->dataChanged();
    MouseCursorManager::restoreOverride();
    if ( curdps_->isEmpty() )
	mErrRet(uiStrings::phrCannotFind(uiStrings::sPosition(mPlural)
								    .toLower()))

    BufferString dpsnm( "Well data:" );
    for ( int idx=0; idx<wellnms.size(); idx++ )
    { dpsnm += wellnms[idx]->buf(); if ( idx!=wellnms.size()-1 ) dpsnm += ","; }

    if ( !attrnms.isEmpty() )
    {
	dpsnm += " / Attributes";
	if ( !extractAttribData(*curdps_,nrlogs) )
	    return false;
    }

    curdps_->setName( dpsnm );
    return true;
}
