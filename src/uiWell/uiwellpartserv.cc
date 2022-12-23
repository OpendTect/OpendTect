/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellpartserv.h"

#include "welltransl.h"
#include "wellman.h"
#include "welldata.h"
#include "welllog.h"
#include "welltrack.h"
#include "welld2tmodel.h"
#include "welldisp.h"
#include "welllogset.h"
#include "wellwriter.h"

#include "uibulkwellimp.h"
#include "uibuttongroup.h"
#include "uid2tmodelgrp.h"
#include "uiioobjselgrp.h"
#include "uiioobjseldlg.h"
#include "uilabel.h"
#include "uilaswriter.h"
#include "uimsg.h"
#include "uisimplemultiwell.h"
#include "uitoolbutton.h"
#include "uiwellrdmlinedlg.h"
#include "uiwelldataexport.h"
#include "uiwelldisplay.h"
#include "uiwelldisppropdlg.h"
#include "uiwelldlgs.h"
#include "uiwelllogimpexp.h"
#include "uiwelllogcalc.h"
#include "uiwellimpasc.h"
#include "uiwelllogtools.h"
#include "uiwellman.h"
#include "uiwellmarkerdlg.h"

#include "arrayndimpl.h"
#include "color.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "multiid.h"
#include "ptrman.h"
#include "survinfo.h"
#include "timedepthmodel.h"


namespace Well {

class DBDisplayProperties
{
public:
	DBDisplayProperties( const DisplayProperties& dpprop,
			     const MultiID& dbkey )
	    : dispprop_(dpprop)
	    , dbkey_(dbkey)
	{}

	DisplayProperties	dispprop_;
	MultiID			dbkey_;
};

}


int uiWellPartServer::evPreviewRdmLine()	    { return 0; }
int uiWellPartServer::evCleanPreview()		    { return 1; }
int uiWellPartServer::evDisplayWell()		    { return 2; }


uiWellPartServer::uiWellPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , disponcreation_(false)
    , randLineDlgClosed(this)
    , uiwellpropDlgClosed(this)
{
    mAttachCB( IOM().surveyChanged, uiWellPartServer::survChangedCB );
}


uiWellPartServer::~uiWellPartServer()
{
    detachAllNotifiers();
    cleanup();
}


void uiWellPartServer::survChangedCB( CallBacker* )
{
    cleanup();
}


void uiWellPartServer::cleanup()
{
    deleteAndZeroPtr( manwelldlg_ );
    deleteAndZeroPtr( impsimpledlg_ );
    deleteAndZeroPtr( impbulktrackdlg_ );
    deleteAndZeroPtr( impbulkdirwelldlg_ );
    deleteAndZeroPtr( impbulklogdlg_ );
    deleteAndZeroPtr( impbulkmrkrdlg_ );
    deleteAndZeroPtr( impbulkd2tdlg_ );
    deleteAndZeroPtr( rdmlinedlg_ );
    deleteAndZeroPtr( wellexpdlg_ );
    deepErase( wellpropdlgs_ );
    deepErase( wellpropcaches_ );
    deleteAndZeroPtr( wellmgrinfodlg_ );

    Well::MGR().cleanup();
}


void uiWellPartServer::bulkImportTrack()
{
    if ( !impbulktrackdlg_ )
	impbulktrackdlg_ = new uiBulkTrackImport( parent() );

    impbulktrackdlg_->show();
}


void uiWellPartServer::bulkImportLogs()
{
    if ( !impbulklogdlg_ )
	impbulklogdlg_ = new uiBulkLogImport( parent() );

    impbulklogdlg_->show();
}

void uiWellPartServer::bulkImportMarkers()
{
    if ( !impbulkmrkrdlg_ )
	impbulkmrkrdlg_ = new uiBulkMarkerImport( parent() );

    impbulkmrkrdlg_->show();
}


void uiWellPartServer::bulkImportD2TModel()
{
    if ( !impbulkd2tdlg_ )
	impbulkd2tdlg_ = new uiBulkD2TModelImport( parent() );

    impbulkd2tdlg_->show();
}


void uiWellPartServer::bulkImportDirectional()
{
    if ( !impbulkdirwelldlg_ )
	impbulkdirwelldlg_ = new uiBulkDirectionalImport( parent() );

    impbulkdirwelldlg_->show();
}


void uiWellPartServer::exportWellData()
{
    if ( !wellexpdlg_ )
	wellexpdlg_ = new uiWellExportFacility( parent(), *this );

    wellexpdlg_->show();
}


void uiWellPartServer::importTrack()
{
    if ( uiwellimpdlg_ )
    {
	uiwellimpdlg_->show();
	uiwellimpdlg_->raise();
	return;
    }

    uiwellimpdlg_ = new uiWellImportAsc( parent() );
    uiwellimpdlg_->importReady.notify(
		mCB(this,uiWellPartServer,importReadyCB) );
    uiwellimpdlg_->show();
}


void uiWellPartServer::exportLogToLAS()
{
    uiLASWriter dlg( parent() );
    dlg.go();
}


void uiWellPartServer::importLogs()
{
    uiImportLogsDlg dlg( parent(), 0, true );
    dlg.go();
}


void uiWellPartServer::importMarkers()
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(Well);
    ctio->ctxt_.forread_ = true;
    uiIOObjSelDlg::Setup sdsu; sdsu.multisel( false );
    uiIOObjSelDlg wellseldlg( parent(), sdsu, *ctio  );
    wellseldlg.setCaption( tr("Import Markers") );
    if ( !wellseldlg.go() ) return;

    const MultiID mid = wellseldlg.chosenID();
    RefMan<Well::Data> wd = Well::MGR().get( mid,
			Well::LoadReqs( Well::Trck, Well::D2T, Well::Mrkrs ) );
    if ( !wd ) return;

    wd->track().setName( wd->name() );
    const Well::MarkerSet origmarkers = wd->markers();
    uiMarkerDlg dlg( parent(), wd->track(), wd->d2TModel() );
    dlg.setMarkerSet( wd->markers() );
    if ( !dlg.go() ) return;

    dlg.getMarkerSet( wd->markers() );
    Well::Writer wtr( mid, *wd );
    if ( !wtr.putMarkers() )
    {
	uiMSG().error( tr("Cannot write new markers to disk") );
	wd->markers() = origmarkers;
    }

    wd->markerschanged.trigger();
}


void uiWellPartServer::showWellMgrInfo()
{
    if ( !wellmgrinfodlg_ )
	wellmgrinfodlg_ = new uiWellMgrInfoDlg( parent() );

    wellmgrinfodlg_->show();
}


void uiWellPartServer::importReadyCB( CallBacker* cb )
{
    if ( uiwellimpdlg_ && cb==uiwellimpdlg_ )
    {
	crwellids_.erase();
	crwellids_.add( uiwellimpdlg_->getWellID() );
	sendEvent( evDisplayWell() );
    }
}


bool uiWellPartServer::selectWells( TypeSet<MultiID>& wellids )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(Well);
    ctio->ctxt_.forread_ = true;
    uiIOObjSelDlg::Setup sdsu; sdsu.multisel( true );
    uiIOObjSelDlg dlg( parent(), sdsu, *ctio  );
    if ( !dlg.go() ) return false;

    wellids.setEmpty();
    dlg.getChosen( wellids );
    return !wellids.isEmpty();
}


bool uiWellPartServer::editDisplayProperties( const MultiID& mid,
					      OD::Color bkCol )
{
    if ( !IOM().implExists(mid) )
	return false;

    allapplied_ = false;
    const int dlgidx = getPropDlgIndex( mid );
    if ( wellpropdlgs_.validIdx(dlgidx) )
	return wellpropdlgs_[dlgidx]->go();

    auto* uiwellpropdlg = new uiWellDispPropDlg( parent(), mid, false, bkCol );
    mAttachCB( uiwellpropdlg->saveReq, uiWellPartServer::saveWellDispProps );
    mAttachCB( uiwellpropdlg->applyTabReq, uiWellPartServer::applyTabProps );
    mAttachCB( uiwellpropdlg->resetAllReq, uiWellPartServer::resetAllProps );
    mAttachCB( uiwellpropdlg->windowClosed,
	       uiWellPartServer::wellPropDlgClosed );
    mAttachCB( uiwellpropdlg->objectToBeDeleted(),
	       uiWellPartServer::wellPropDlgToBeDeleted );
    wellpropdlgs_.add( uiwellpropdlg );
    uiwellpropdlg->go();

    return true;
}


int uiWellPartServer::getPropDlgIndex( const MultiID& mid )
{
    for ( int idx=0; idx<wellpropdlgs_.size(); idx++ )
    {
	if ( !wellpropdlgs_[idx]->wellData() )
	    continue;

	const MultiID dlgid = wellpropdlgs_[idx]->wellData()->multiID();
	if ( dlgid == mid )
	    return idx;
    }

    return -1;
}


void uiWellPartServer::saveWellDispProps( CallBacker* cb )
{
    mDynamicCastGet(uiWellDispPropDlg*,dlg,cb)
    if ( !dlg ) { pErrMsg("Huh"); return; }

    ConstRefMan<Well::Data> edwd = dlg->wellData();
    if ( allapplied_ )
	saveAllWellDispProps();
    else
    {
	saveWellDispProps( *edwd.ptr() );
	if ( edwd )
	    dlg->setNeedsSave( false );
    }

    if ( dlg->saveButtonChecked() && edwd )
    {
	const Well::DisplayProperties& edprops = edwd->displayProperties();
	edprops.defaults() = edprops;
	edprops.commitDefaults();
    }
}


void uiWellPartServer::saveAllWellDispProps()
{
    const auto& wds = Well::MGR().wells();
    for ( int idx=0; idx<wds.size(); idx++ )
    {
	ConstRefMan<Well::Data> curwd = wds[idx];
	saveWellDispProps( *curwd.ptr() );
    }

    for ( auto* wellpropdlgs : wellpropdlgs_ )
	wellpropdlgs->setNeedsSave( false );
}


void uiWellPartServer::saveWellDispProps( const Well::Data& wd )
{
    Well::Writer wr( wd.multiID(), wd );
    if ( wr.putDispProps() )
    {
	auto& wdedptr = const_cast<Well::Data&>( wd );
	wdedptr.displayProperties(true).setValid( true );
	wdedptr.displayProperties(false).setValid( true );
	wdedptr.displayProperties(true).setModified( false );
	wdedptr.displayProperties(false).setModified( false );
    }
    else
    {
	uiMSG().error(tr("Could not write display properties for \n%1")
		    .arg(wd.name()));
    }
}


void uiWellPartServer::applyTabProps( CallBacker* cb )
{
    mDynamicCastGet(uiWellDispPropDlg*,dlg,cb)
    if ( !dlg ) { pErrMsg("Huh"); return; }
    uiWellDispPropDlg::TabType pageid = dlg->currentTab();
    const bool is2d = dlg->is2D();

    ConstRefMan<Well::Data> edwd = dlg->wellData();
    const Well::DisplayProperties& edprops = edwd->displayProperties();

    auto& wells = Well::MGR().wells();
    for ( int ikey=0; ikey<wells.size(); ikey++ )
    {
	RefMan<Well::Data> wd = wells[ikey];
	if ( wd && wd != edwd )
	{
	    const MultiID wllkey = wd->multiID();
	    Well::DisplayProperties& wdprops = wd->displayProperties( is2d );

	    Well::DBDisplayProperties* wdpropscache = nullptr;
	    for ( auto* wellpropscache : wellpropcaches_ )
	    {
		if ( wellpropscache->dbkey_ == wllkey &&
		     wellpropscache->dispprop_.is2D() == is2d )
		{
		    wdpropscache = wellpropscache;
		    break;
		}
	    }
	    if ( !wdpropscache )
		wellpropcaches_.add(
			new Well::DBDisplayProperties( wdprops,wllkey ) );

	    switch ( pageid )
	    {
		case uiWellDispPropDlg::Track:
		    wdprops.setTrack( edprops.getTrack() );
		    break;
		case uiWellDispPropDlg::Marker:
		    wdprops.setMarkers( wd, edprops.getMarkers() );
		    break;
		case uiWellDispPropDlg::LeftLog:
		    wdprops.setLeftLog( wd, edprops.getLogs().left_ );
		    break;
		case uiWellDispPropDlg::CenterLog:
		    wdprops.setCenterLog( wd, edprops.getLogs().center_ );
		    break;
		case uiWellDispPropDlg::RightLog:
		    wdprops.setRightLog( wd, edprops.getLogs().right_ );
	    }
	    is2d ? wd->disp2dparschanged.trigger()
		 : wd->disp3dparschanged.trigger();
	}
    }
    allapplied_ = true;
    if ( !edprops.isModified() )
	dlg->setNeedsSave( true );
}


void uiWellPartServer::resetAllProps( CallBacker* cb )
{
    mDynamicCastGet(uiWellDispPropDlg*,dlg,cb)
    if ( !dlg ) { pErrMsg("Huh"); return; }
    const bool is2d = dlg->is2D();

    auto& wells = Well::MGR().wells();
    for ( int ikey=0; ikey<wells.size(); ikey++ )
    {
	RefMan<Well::Data> wd = wells[ikey];
	if ( !wd )
	    continue;

	const MultiID wllkey = wd->multiID();
	if ( Well::MGR().reloadDispPars(wllkey,is2d) )
	    continue;

	const Well::DBDisplayProperties* wdpropcache = nullptr;
	for ( const auto* wellpropcache : wellpropcaches_ )
	{
	    if ( wellpropcache->dbkey_ == wllkey &&
		 wellpropcache->dispprop_.is2D() == is2d )
	    {
		wdpropcache = wellpropcache;
		break;
	    }
	}

	if ( !wdpropcache )
	    continue;

	wd->displayProperties(is2d) = wdpropcache->dispprop_;
	is2d ? wd->disp2dparschanged.trigger()
	     : wd->disp3dparschanged.trigger();
    }

    allapplied_ = false;
    for ( auto* wellpropdlgs : wellpropdlgs_ )
	wellpropdlgs->setNeedsSave( false );
}


void uiWellPartServer::closePropDlg( const MultiID& mid )
{
    const int dlgidx = getPropDlgIndex( mid );
    if ( wellpropdlgs_.validIdx(dlgidx) )
	delete wellpropdlgs_.removeSingle( dlgidx );
}


void uiWellPartServer::wellPropDlgClosed( CallBacker* )
{
    sendEvent( evCleanPreview() );
    uiwellpropDlgClosed.trigger();
}


void uiWellPartServer::wellPropDlgToBeDeleted( CallBacker* cb )
{
    mDynamicCastGet(uiWellDispPropDlg*,dlg,cb)
    if ( !dlg )
	return;

    for ( int idx=wellpropdlgs_.size()-1; idx>=0; idx-- )
    {
	uiWellDispPropDlg* uiwellpropdlg = wellpropdlgs_[idx];
	if ( uiwellpropdlg != dlg )
	    continue;

	wellpropdlgs_.removeSingle( idx );
    }
}


void uiWellPartServer::displayIn2DViewer( const MultiID& mid )
{
    auto* welldispwin = new uiWellDisplayWin( parent(), mid );
    welldispwin->setDeleteOnClose( true );
    welldispwin->show();
}


bool uiWellPartServer::hasLogs( const MultiID& wellid ) const
{
    ConstRefMan<Well::Data> wd = Well::MGR().get( wellid,
					    Well::LoadReqs( Well::LogInfos ) );
    return wd && wd->logs().size();
}


void uiWellPartServer::getLogNames( const MultiID& wellid,
					BufferStringSet& lognms ) const
{
    lognms.erase();
    Well::MGR().getLogNamesByID( wellid, lognms );
}


void uiWellPartServer::manageWells()
{
    delete manwelldlg_;
    manwelldlg_ = new uiWellMan( parent() );
    new uiToolButton( manwelldlg_->extraButtonGroup(), "multisimplewell",
		      tr("Import well locations"),
		      mCB(this,uiWellPartServer,simpImp) );
    manwelldlg_->go();
}


class uiWellRockPhysLauncher : public uiDialog
{ mODTextTranslationClass(uiWellRockPhysLauncher)
public:

uiWellRockPhysLauncher( uiParent* p )
    : uiDialog( p, Setup(tr("Rock Physics - Well Logs"),
		tr("Select one or more wells to add well logs to"),
		mODHelpKey(mWellRockPhysLauncherHelpID)))
{
    selgrp_ = new uiIOObjSelGrp( this, mIOObjContext(Well),
			uiIOObjSelGrp::Setup(OD::ChooseAtLeastOne) );
}

bool acceptOK( CallBacker* ) override
{
    mids_.erase();
    selgrp_->getChosen( mids_ );
    if ( mids_.isEmpty() )
    {
	uiMSG().error( tr("Please select at least 1 well.") );
	return false;
    }

    return true;
}

    TypeSet<MultiID>	mids_;
    uiIOObjSelGrp*	selgrp_;

};


void uiWellPartServer::launchRockPhysics()
{
    TypeSet<MultiID> keys;
    Well::MGR().getWellKeys( keys );
    if ( keys.isEmpty() )
    {
	uiMSG().error( tr("Please create one or more wells first.") );
	return;
    }

    if ( keys.size() > 1 )
    {
	uiWellRockPhysLauncher dlg( parent() );
	if ( !dlg.go() )
	    return;

	keys = dlg.mids_;
    }

    uiWellLogCalc lcdlg( parent(), keys, true );
    lcdlg.go();
}


void uiWellPartServer::simpImp( CallBacker* cb )
{
    if ( !impsimpledlg_ )
    {
	impsimpledlg_ = new uiSimpleMultiWellCreate( parent() );
	impsimpledlg_->windowClosed.notify(
		mCB(this,uiWellPartServer,simpleImpDlgClosed) );
    }

    impsimpledlg_->show();
}


void uiWellPartServer::simpleImpDlgClosed( CallBacker* )
{
    if ( !impsimpledlg_ )
	return;

    crwellids_ = impsimpledlg_->createdWellIDs();
    if ( crwellids_.isEmpty() )
	return;

    if ( impsimpledlg_->wantDisplay() )
	sendEvent( evDisplayWell() );

    if ( !manwelldlg_ )
	return;

    manwelldlg_->selGroup()->fullUpdate( crwellids_.get(0) );
}


void uiWellPartServer::doLogTools()
{
    uiWellLogToolWinMgr tooldlg( parent() );
    tooldlg.go();
}


void uiWellPartServer::selectWellCoordsForRdmLine()
{
    delete rdmlinedlg_;
    rdmlinedlg_ = new uiWell2RandomLineDlg( parent(), this );
    rdmlinedlg_->objectToBeDeleted().notify(
			mCB(this,uiWellPartServer,rdmlnDlgDeleted));
    rdmlinedlg_->windowClosed.notify(mCB(this,uiWellPartServer,rdmlnDlgClosed));
    rdmlinedlg_->go();
}


void uiWellPartServer::rdmlnDlgDeleted( CallBacker* )
{
    rdmlinedlg_ = 0;
}


void uiWellPartServer::rdmlnDlgClosed( CallBacker* )
{
    multiid_ = rdmlinedlg_->getRandLineID();
    disponcreation_ = rdmlinedlg_->dispOnCreation();
    sendEvent( evCleanPreview() );
    randLineDlgClosed.trigger();
}


void uiWellPartServer::sendPreviewEvent()
{
    sendEvent( evPreviewRdmLine() );
}


void uiWellPartServer::getRdmLineCoordinates( TypeSet<Coord>& coords )
{
    rdmlinedlg_->getCoordinates( coords );
}


bool uiWellPartServer::setupNewWell( BufferString& wellname,
							OD::Color& wellcolor )
{
    uiNewWellDlg dlg( parent() );
    dlg.go();
    wellname = dlg.getWellName();
    wellcolor = dlg.getWellColor();
    return ( dlg.uiResult() == 1 );
}


static void makeTimeDepthModel( bool addwellhead, double z0, double srddepth,
				const Well::Info& info, TimeDepthModel& model )
{
    TypeSet<float> dpths, times;
    const bool wllheadbelowsrd = !addwellhead && z0 > 0.;
    const double vrepl( info.replvel_ );
    const UnitOfMeasure* zun = UnitOfMeasure::surveyDefDepthUnit();
    double vtmp = (double)uiD2TModelGroup::getDefaultTemporaryVelocity();
    if ( SI().zIsTime() && SI().depthsInFeet() && zun )
	vtmp = zun->internalValue( vtmp );

    double originz = srddepth;
    if ( wllheadbelowsrd )
	originz += vrepl * z0 / 2.;

    dpths.setSize( 3, (float)originz );
    times.setSize( 3, wllheadbelowsrd ? mCast(float,z0) : 0.f );
    dpths[0] -= (float)vtmp; times[0] -= mCast(float, 2. * vtmp / vrepl);
    dpths[2] += (float)vtmp; times[2] += 2.f;
    model.setModel( dpths.arr(), times.arr(), dpths.size() );
}


#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiWellPartServer::storeWell( const TypeSet<Coord3>& coords,
				  const char* wellname, MultiID& mid,
				  bool addwellhead )
{
    if ( coords.isEmpty() )
	mErrRet(tr("Empty well track"))

    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(Well);
    ctio->setObj(0); ctio->setName( wellname );
    if ( !ctio->fillObj() )
	mErrRet( uiStrings::phrCannotCreateDBEntryFor( toUiString(wellname) ))

    RefMan<Well::Data> well = new Well::Data( wellname );
    Well::Track& track = well->track();
    TypeSet<Coord3> pos = coords;
    const double srddepth = -1. * SI().seismicReferenceDatum();
    const double refz = SI().zIsTime() ? 0. : srddepth;
    if ( coords[0].z > refz+mDefEps && addwellhead )
	pos.insert( 0, Coord3( coords[0].x, coords[0].y, refz ) );

    well->info().surfacecoord_ = Coord( pos[0].x, pos[0].y );

    TimeDepthModel tdmodel;
    if ( SI().zIsTime() )
    {
	makeTimeDepthModel( addwellhead, pos[0].z, srddepth, well->info(),
			    tdmodel );
	for ( int idx=0; idx<pos.size(); idx++ )
	    pos[idx].z = mCast( float,tdmodel.getDepth( (float)pos[idx].z ) );
    }

    track.addPoint( pos[0], 0.f );
    for ( int idx=1; idx<pos.size(); idx++ )
    {
	const float dist = mCast( float, pos[idx].distTo( pos[idx-1] ) );
	track.addPoint( pos[idx], track.dah(idx-1) + dist );
    }

    const Interval<double> trackrg = track.zRangeD();
    const double startz = mMAX( srddepth, trackrg.start );
    if ( SI().zIsTime() && trackrg.stop > startz )
    {
	Well::D2TModel* d2t = new Well::D2TModel;
	const float startdah = trackrg.start > srddepth
			      ? 0.f : track.getDahForTVD(srddepth);
	d2t->add( startdah, tdmodel.getTime( (float)startz ) );

	const float stopdah = track.getDahForTVD( trackrg.stop );
	d2t->add( stopdah, tdmodel.getTime( (float)trackrg.stop ) );

	well->setD2TModel( d2t );
    }

    Well::Writer wwr( *ctio->ioobj_, *well );
    if ( !wwr.put() )
	mErrRet( mToUiStringTodo(wwr.errMsg()) )

    mid = ctio->ioobj_->key();
    if ( manwelldlg_ )
	manwelldlg_->selGroup()->fullUpdate( -1 );

    delete ctio->ioobj_;
    return true;
}
