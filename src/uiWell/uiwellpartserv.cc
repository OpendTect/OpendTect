/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiwellpartserv.h"

#include "velocitycalc.h"
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

#include "arrayndimpl.h"
#include "color.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "multiid.h"
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
    , multiid_(0)
    , randLineDlgClosed(this)
    , uiwellpropDlgClosed(this)
    , manwelldlg_(0)
    , impsimpledlg_(0)
    , impbulktrackdlg_(0)
    , impbulklogdlg_(0)
    , impbulkmrkrdlg_(0)
    , impbulkd2tdlg_(0)
{
    IOM().surveyChanged.notify( mCB(this,uiWellPartServer,survChangedCB) );
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
    delete manwelldlg_; manwelldlg_ = 0;
    delete impsimpledlg_; impsimpledlg_ = 0;
    delete impbulktrackdlg_; impbulktrackdlg_ = 0;
    delete impbulklogdlg_; impbulklogdlg_ = 0;
    delete impbulkmrkrdlg_; impbulkmrkrdlg_ = 0;
    delete impbulkd2tdlg_; impbulkd2tdlg_ = 0;
    delete rdmlinedlg_; rdmlinedlg_ = 0;
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

    const MultiID mid = wellseldlg.chosenID();
    RefMan<Well::Data> wd = Well::MGR().get( mid );
    if ( !wd ) return;

    const Well::MarkerSet origmarkers = wd->markers();
    uiMarkerDlg dlg( parent(), wd->track() );
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


void uiWellPartServer::importReadyCB( CallBacker* cb )
{
    if ( uiwellimpdlg_ && cb==uiwellimpdlg_ )
    {
	crwellids_.erase();
	crwellids_.add( uiwellimpdlg_->getWellID().buf() );
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


bool uiWellPartServer::editDisplayProperties( const MultiID& mid )
{
    allapplied_ = false;
    RefMan<Well::Data> wd = Well::MGR().get( mid );
    if ( !wd ) return false;

    const int dlgidx = getPropDlgIndex( mid );
    if ( dlgidx != -1 )
    {
	uiWellDispPropDlg* dispdlg = wellpropdlgs_[dlgidx];
	dispdlg->updateLogs();
	return dispdlg->go();
    }

    uiWellDispPropDlg* uiwellpropdlg = new uiWellDispPropDlg( parent(), wd );
    uiwellpropdlg->applyAllReq.notify( mCB(this,uiWellPartServer,applyAll) );
    uiwellpropdlg->windowClosed.notify(
			mCB(this,uiWellPartServer, wellPropDlgClosed) );
    wellpropdlgs_ += uiwellpropdlg;
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


void uiWellPartServer::closePropDlg( const MultiID& mid )
{
    const int dlgidx = getPropDlgIndex( mid );
    if ( dlgidx != -1 )
	delete wellpropdlgs_.removeSingle( dlgidx );
}


void uiWellPartServer::wellPropDlgClosed( CallBacker* cb)
{
    mDynamicCastGet(uiWellDispPropDlg*,dlg,cb)
    if ( !dlg ) { pErrMsg("Huh"); return; }
    ConstRefMan<Well::Data> edwd = dlg->wellData();
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


void uiWellPartServer::saveWellDispProps( const Well::Data* wd )
{
    ObjectSet<Well::Data>& wds = Well::MGR().wells();
    for ( int iwll=0; iwll<wds.size(); iwll++ )
    {
	Well::Data& curwd = *wds[iwll];
	if ( wd && &curwd != wd )
	   continue;
	saveWellDispProps( curwd, curwd.multiID() );
    }
}


void uiWellPartServer::saveWellDispProps(const Well::Data& w,const MultiID& key)
{
    Well::Writer wr( key, w );
    if ( !wr.putDispProps() )
	uiMSG().error(tr("Could not write display properties for \n%1")
		    .arg(w.name()));
}


void uiWellPartServer::applyAll( CallBacker* cb )
{
    mDynamicCastGet(uiWellDispPropDlg*,dlg,cb)
    if ( !dlg ) { pErrMsg("Huh"); return; }
    ConstRefMan<Well::Data> edwd = dlg->wellData();
    const Well::DisplayProperties& edprops = edwd->displayProperties();

    ObjectSet<Well::Data>& wds = Well::MGR().wells();
    for ( int iwll=0; iwll<wds.size(); iwll++ )
    {
	Well::Data& wd = *wds[iwll];
	if ( &wd != edwd )
	{
	    wd.displayProperties() = edprops;
	    wd.disp3dparschanged.trigger();
	}
    }
    allapplied_ = true;
}


void uiWellPartServer::displayIn2DViewer( const MultiID& mid )
{
    RefMan<Well::Data> wd = Well::MGR().get( mid );
    if ( !wd ) return;

    uiWellDisplayWin* welldispwin = new uiWellDisplayWin( parent(), *wd );
    welldispwin->show();
}


bool uiWellPartServer::hasLogs( const MultiID& wellid ) const
{
    ConstRefMan<Well::Data> wd = Well::MGR().get( wellid );
    return wd && wd->logs().size();
}


void uiWellPartServer::getLogNames( const MultiID& wellid,
					BufferStringSet& lognms ) const
{
    ConstRefMan<Well::Data> wd = Well::MGR().get( wellid );
    if ( !wd ||  wd->logs().isEmpty() ) return;
    for ( int idx=0; idx<wd->logs().size(); idx++ )
	lognms.add( wd->logs().getLog(idx).name() );
}


void uiWellPartServer::manageWells()
{
    delete manwelldlg_;
    manwelldlg_ = new uiWellMan( parent() );
    new uiToolButton( manwelldlg_->extraButtonGroup(), "multisimplewell",
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
    selgrp_ = new uiIOObjSelGrp( this, mIOObjContext(Well),
			uiIOObjSelGrp::Setup(OD::ChooseAtLeastOne) );
}

bool acceptOK( CallBacker* )
{
    TypeSet<MultiID> mids;
    selgrp_->getChosen( mids );
    if ( mids.isEmpty() )
	return false;

    uiWellLogCalc dlg( this, mids, true );
    dlg.go();
    return true;
}

    uiIOObjSelGrp*	selgrp_;

};


void uiWellPartServer::launchRockPhysics()
{
    uiWellRockPhysLauncher dlg( parent() );
    const int sz = dlg.selgrp_->size();
    if ( sz == 0 )
	uiMSG().error( tr("Please create one or more wells first") );
    else if ( sz > 1 )
	dlg.go();
    else
    {
	dlg.selgrp_->chooseAll();
	TypeSet<MultiID> mids; dlg.selgrp_->getChosen( mids );
	uiWellLogCalc lcdlg( parent(), mids, true );
	lcdlg.go();
    }
}


void uiWellPartServer::simpImp( CallBacker* cb )
{
    if ( !impsimpledlg_ )
	impsimpledlg_ = new uiSimpleMultiWellCreate( parent() );

    if ( !impsimpledlg_->go() )
	return;

    crwellids_ = impsimpledlg_->createdWellIDs();
    if ( crwellids_.isEmpty() ) return;

    if ( impsimpledlg_->wantDisplay() )
	sendEvent( evDisplayWell() );

    mDynamicCastGet(uiToolButton*,tb,cb)
    uiMainWin* mw = tb ? tb->mainwin() : 0;
    mDynamicCastGet(uiWellMan*,wm,mw)
    if ( !wm ) return;

    wm->selGroup()->fullUpdate( MultiID(crwellids_.get(0)) );
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
    rdmlinedlg_->tobeDeleted.notify(mCB(this,uiWellPartServer,rdmlnDlgDeleted));
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


bool uiWellPartServer::setupNewWell( BufferString& wellname, Color& wellcolor )
{
    uiNewWellDlg dlg( parent() );
    dlg.go();
    wellname = dlg.getName();
    wellcolor = dlg.getWellColor();
    return ( dlg.uiResult() == 1 );
}


static void makeTimeDepthModel( bool addwellhead, double z0, double srddepth,
				const Well::Info& info, TimeDepthModel& model )
{
    TypeSet<float> dpths, times;
    const bool wllheadbelowsrd = !addwellhead && z0 > 0.;
    const double vrepl = mCast(double,info.replvel);
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

    well->info().surfacecoord = Coord( pos[0].x, pos[0].y );

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
