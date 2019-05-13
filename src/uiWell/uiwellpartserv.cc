/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2003
________________________________________________________________________

-*/


#include "uiwellpartserv.h"

#include "velocitycalc.h"
#include "welltransl.h"
#include "wellmanager.h"
#include "welllogset.h"
#include "welltrack.h"
#include "wellinfo.h"
#include "welld2tmodel.h"
#include "welldisp.h"
#include "wellwriter.h"

#include "uibulkwellimp.h"
#include "uibuttongroup.h"
#include "uid2tmodelgrp.h"
#include "uiioobjseldlg.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uisimplemultiwell.h"
#include "uitoolbutton.h"
#include "uiwellrdmlinedlg.h"
#include "uiwelldisplay.h"
#include "uiwelldisppropdlg.h"
#include "uiwelldlgs.h"
#include "uiwelllogimpexp.h"
#include "uiwelllogcalc.h"
#include "uiwellimpasc.h"
#include "uiwelllogtools.h"
#include "uiwellman.h"
#include "uiwellmarkerdlg.h"
#include "uiwellsel.h"

#include "arrayndimpl.h"
#include "color.h"
#include "ctxtioobj.h"
#include "dbman.h"
#include "ioobj.h"
#include "dbkey.h"
#include "ptrman.h"
#include "survinfo.h"


int uiWellPartServer::evPreviewRdmLine()	    { return 0; }
int uiWellPartServer::evCleanPreview()		    { return 1; }
int uiWellPartServer::evDisplayWell()		    { return 2; }


uiWellPartServer::uiWellPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , rdmlinedlg_(0)
    , uiwellimpdlg_(0)
    , disponcreation_(false)
    , randLineDlgClosed(this)
    , uiwellpropDlgClosed(this)
    , manwelldlg_(0)
    , impsimpledlg_(0)
    , impbulktrackdlg_(0)
    , impbulklogdlg_(0)
    , impbulkmrkrdlg_(0)
    , impbulkd2tdlg_(0)
{
    DBM().surveyChanged.notify( mCB(this,uiWellPartServer,survChangedCB) );
}


uiWellPartServer::~uiWellPartServer()
{
    delete rdmlinedlg_;
    delete manwelldlg_;
    delete impsimpledlg_;
    delete impbulktrackdlg_;
    delete impbulklogdlg_;
    delete impbulkmrkrdlg_;
    delete impbulkd2tdlg_;
    deepErase( wellpropdlgs_ );
}


void uiWellPartServer::survChangedCB( CallBacker* )
{
    deleteAndZeroPtr( manwelldlg_ );
    deleteAndZeroPtr( impsimpledlg_ );
    deleteAndZeroPtr( impbulktrackdlg_ );
    deleteAndZeroPtr( impbulklogdlg_ );
    deleteAndZeroPtr( impbulkmrkrdlg_ );
    deleteAndZeroPtr( impbulkd2tdlg_ );
    deleteAndZeroPtr( rdmlinedlg_ );
    deepErase( wellpropdlgs_ );
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


void uiWellPartServer::importLogs()
{
    uiImportLogsDlg dlg( parent(), 0 );
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

    const DBKey mid = wellseldlg.chosenID();
    RefMan<Well::Data> wd = Well::MGR().fetchForEdit( mid );
    if ( !wd )
	return;

    wd->track().setName( wd->name() );
    const Well::MarkerSet origmarkers = wd->markers();
    uiMarkerDlg dlg( parent(), wd->track() );
    dlg.setMarkerSet( wd->markers() );
    if ( !dlg.go() )
	return;

    dlg.getMarkerSet( wd->markers() );
    SilentTaskRunnerProvider trprov;
    uimsg().handleErrors( Well::MGR().save(mid,trprov) );
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


bool uiWellPartServer::selectWells( DBKeySet& wellids )
{
    const uiDialog::Setup setup( uiStrings::phrLoad(tr("Well(s)")),
				 uiStrings::phrSelect(tr("input Well(s)")),
				 mNoHelpKey );
    uiDialog dlg( parent(), setup );
    PtrMan<uiMultiWellSel> multiwellsel = new uiMultiWellSel( &dlg, false );
    if ( !dlg.go() )
	return false;

    multiwellsel->getSelected( wellids );
    return !wellids.isEmpty();
}


bool uiWellPartServer::editDisplayProperties( const DBKey& mid )
{
    allapplied_ = false;
    RefMan<Well::Data> wd = Well::MGR().fetchForEdit( mid );
    if ( !wd ) return false;

    const int dlgidx = getPropDlgIndex( mid );
    if ( dlgidx != -1 )
    {
	uiWellDispPropDlg* dispdlg = wellpropdlgs_[dlgidx];
	dispdlg->welldisppropgrp_->updateLogs();
	return dispdlg->go();
    }

    uiWellDispPropDlg* uiwellpropdlg = new uiWellDispPropDlg( parent(), wd );
    uiwellpropdlg->welldisppropgrp_->applyAllReq.notify(
					mCB(this,uiWellPartServer,applyAll) );
    uiwellpropdlg->windowClosed.notify(
			mCB(this,uiWellPartServer, wellPropDlgClosed) );
    wellpropdlgs_ += uiwellpropdlg;
    uiwellpropdlg->go();
    return true;
}


int uiWellPartServer::getPropDlgIndex( const DBKey& mid )
{
    for ( int idx=0; idx<wellpropdlgs_.size(); idx++ )
    {
	if ( !wellpropdlgs_[idx]->welldisppropgrp_->wellData() )
	    continue;

	Well::Data* wd = wellpropdlgs_[idx]->welldisppropgrp_->wellData();
	const DBKey dlgid = wd->dbKey();
	if ( dlgid == mid )
	    return idx;
    }

    return -1;
}


void uiWellPartServer::closePropDlg( const DBKey& mid )
{
    const int dlgidx = getPropDlgIndex( mid );
    if ( dlgidx != -1 )
	delete wellpropdlgs_.removeSingle( dlgidx );
}


void uiWellPartServer::wellPropDlgClosed( CallBacker* cb )
{
    mDynamicCastGet(uiWellDispPropDlg*,dlg,cb)
    if ( !dlg ) { pErrMsg("Huh"); return; }
    ConstRefMan<Well::Data> edwd = dlg->welldisppropgrp_->wellData();
    if ( !edwd ) { pErrMsg("well data has been deleted"); return; }
    const Well::DisplayProperties& edprops = edwd->displayProperties();

    if ( dlg->savedefault_ == true )
    {
	edprops.defaults() = edprops;
	saveWellDispProps( allapplied_ ? 0 : edwd );
	edprops.commitDefaults();
    }

    sendEvent( evCleanPreview() );
    uiwellpropDlgClosed.trigger();
}


void uiWellPartServer::saveWellDispProps( const Well::Data* onlywd )
{
    DBKeySet kys; Well::MGR().getAll( kys, true );
    for ( int idx=0; idx<kys.size(); idx++ )
    {
	const DBKey ky = kys[idx];
	ConstRefMan<Well::Data> wd = Well::MGR().fetch( ky );
	SilentTaskRunnerProvider trprov;
	if ( !onlywd || wd == onlywd )
	    Well::MGR().save( ky, trprov );
    }
}


void uiWellPartServer::applyAll( CallBacker* cb )
{
    mDynamicCastGet(uiWellDispPropDlg*,dlg,cb)
    if ( !dlg ) { pErrMsg("Huh"); return; }
    ConstRefMan<Well::Data> edwd = dlg->welldisppropgrp_->wellData();
    const Well::DisplayProperties& edprops = edwd->displayProperties();

    DBKeySet dbkys; Well::MGR().getAll( dbkys, true );
    for ( int iwll=0; iwll<dbkys.size(); iwll++ )
    {
	RefMan<Well::Data> wd = Well::MGR().fetchForEdit( dbkys[iwll] );
	if ( wd != edwd )
	    wd->displayProperties() = edprops;
    }
    allapplied_ = true;
}


void uiWellPartServer::displayIn2DViewer( const DBKey& mid )
{
    uiWellDisplayWin* welldispwin = new uiWellDisplayWin( parent(), mid );
    welldispwin->show();
}


bool uiWellPartServer::hasLogs( const DBKey& wellid ) const
{
    BufferStringSet nms; Well::MGR().getLogNames( wellid, nms );
    return !nms.isEmpty();
}


void uiWellPartServer::getLogNames( const DBKey& wellid,
					BufferStringSet& lognms ) const
{
    Well::MGR().getLogNames( wellid, lognms );
}


void uiWellPartServer::manageWells()
{
    delete manwelldlg_;
    manwelldlg_ = new uiWellMan( parent() );
    new uiToolButton( manwelldlg_->extraButtonGroup(), "multiwell",
		      tr("Create multiple simple wells"),
		      mCB(this,uiWellPartServer,simpImp) );
    manwelldlg_->go();
}


class uiWellRockPhysLauncher : public uiDialog
{ mODTextTranslationClass(uiWellRockPhysLauncher);
public:

uiWellRockPhysLauncher( uiParent* p )
    : uiDialog( p, Setup(tr("Rock Physics - Well Logs"),
		tr("Select one or more wells to add well logs to"),
                mODHelpKey(mWellRockPhysLauncherHelpID)))
{
    multiwellsel_ = new uiMultiWellSel( this, false );
}

bool acceptOK()
{
    DBKeySet mids;
    multiwellsel_->getSelected( mids );
    if ( mids.isEmpty() )
	return false;

    uiWellLogCalc dlg( this, mids, true );
    dlg.go();
    return true;
}

    uiMultiWellSel*	multiwellsel_;

};


void uiWellPartServer::launchRockPhysics()
{
    uiWellRockPhysLauncher dlg( parent() );
    const int sz = dlg.multiwellsel_->nrWells();
    if ( sz == 0 )
	uimsg().error( uiStrings::phrCreate(tr("one or more wells first")) );
    else if ( sz > 1 )
	dlg.go();
    else
    {
	const DBKeySet dbkys( dlg.multiwellsel_->currentID() );
	uiWellLogCalc lcdlg( parent(), dbkys, true );
	lcdlg.go();
    }
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
			mCB(this,uiWellPartServer,rdmlnDlgDeleted) );
    rdmlinedlg_->windowClosed.notify(mCB(this,uiWellPartServer,rdmlnDlgClosed));
    rdmlinedlg_->go();
}


void uiWellPartServer::rdmlnDlgDeleted( CallBacker* )
{
    rdmlinedlg_ = 0;
}


void uiWellPartServer::rdmlnDlgClosed( CallBacker* )
{
    dbkey_ = rdmlinedlg_->getRandLineID();
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


bool uiWellPartServer::setupNewWell( BufferString& wellname, Color& wellcolor )
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
    TypeSet<double> dpths, times;
    const bool wllheadbelowsrd = !addwellhead && z0 > 0.;
    const double vrepl = info.replacementVelocity();
    const UnitOfMeasure* zun = UnitOfMeasure::surveyDefDepthUnit();
    double vtmp = (double)uiD2TModelGroup::getDefaultTemporaryVelocity();
    if ( SI().zIsTime() && SI().depthsInFeet() && zun )
	vtmp = zun->internalValue( vtmp );

    double originz = srddepth;
    if ( wllheadbelowsrd )
	originz += vrepl * z0 / 2.;

    dpths.setSize( 3, originz );
    times.setSize( 3, wllheadbelowsrd ? z0 : 0. );
    dpths[0] -= vtmp; times[0] -= 2. * vtmp / vrepl;
    dpths[2] += vtmp; times[2] += 2.;
    model.setModel( dpths.arr(), times.arr(), dpths.size() );
}


#define mErrRet(s) { uimsg().error(s); return false; }


bool uiWellPartServer::storeWell( const TypeSet<Coord3>& coords,
				  const char* wellname, DBKey& mid,
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
    if ( coords[0].z_ > refz+mDefEps && addwellhead )
	pos.insert( 0, Coord3( coords[0].x_, coords[0].y_, refz ) );

    well->info().setSurfaceCoord( Coord( pos[0].x_, pos[0].y_ ) );

    TimeDepthModel tdmodel;
    if ( SI().zIsTime() )
    {
	makeTimeDepthModel( addwellhead, pos[0].z_, srddepth, well->info(),
			    tdmodel );
	for ( int idx=0; idx<pos.size(); idx++ )
	    pos[idx].z_ = mCast( float,tdmodel.getDepth( (float)pos[idx].z_ ) );
    }

    track.addPoint( pos[0], 0.f );
    for ( int idx=1; idx<pos.size(); idx++ )
    {
	const float dist = pos[idx].distTo<float>( pos[idx-1] );
	track.addPoint( pos[idx], track.dahByIdx(idx-1) + dist );
    }

    const Interval<double> trackrg = track.zRangeD();
    const double startz = mMAX( srddepth, trackrg.start );
    if ( SI().zIsTime() && trackrg.stop > startz )
    {
	Well::D2TModel& d2t = well->d2TModel();
	const float startdah = trackrg.start > srddepth
			      ? 0.f : track.getDahForTVD(srddepth);
	d2t.setValueAt( startdah, tdmodel.getTime( (float)startz ) );

	const float stopdah = track.getDahForTVD( trackrg.stop );
	d2t.setValueAt( stopdah, tdmodel.getTime( (float)trackrg.stop ) );
    }

    SilentTaskRunnerProvider trprov;
    const uiRetVal uirv =
		    Well::MGR().store( *well, ctio->ioobj_->key(), trprov );
    if ( !uirv.isOK() )
	mErrRet( uirv );

    mid = ctio->ioobj_->key();
    if ( manwelldlg_ )
	manwelldlg_->selGroup()->fullUpdate( -1 );

    delete ctio->ioobj_;
    return true;
}
