/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


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

#include "uiamplspectrum.h"
#include "uibulkwellimp.h"
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

#include "arrayndimpl.h"
#include "color.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "multiid.h"
#include "pixmap.h"
#include "ptrman.h"
#include "survinfo.h"


int uiWellPartServer::evPreviewRdmLine()	    { return 0; }
int uiWellPartServer::evCleanPreview()		    { return 1; }
int uiWellPartServer::evDisplayWell()		    { return 2; }


uiWellPartServer::uiWellPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , rdmlinedlg_(0)
    , uiwellpropdlg_(0)
    , uiwellimpdlg_(0)
    , cursceneid_(-1)
    , disponcreation_(false)
    , multiid_(0)
    , randLineDlgClosed(this)
    , uiwellpropDlgClosed(this)
    , isdisppropopened_(false)
{
}


uiWellPartServer::~uiWellPartServer()
{
    delete rdmlinedlg_;
    Well::MGR().removeAll();
}


void uiWellPartServer::bulkImportTrack()
{
    uiBulkTrackImport dlg( parent() );
    dlg.go();
}


void uiWellPartServer::bulkImportLogs()
{
    uiBulkLogImport dlg( parent() );
    dlg.go();
}

void uiWellPartServer::bulkImportMarkers()
{
    uiBulkMarkerImport dlg( parent() );
    dlg.go();
}


void uiWellPartServer::bulkImportD2TModel()
{
    uiBulkD2TModelImport dlg( parent() );
    dlg.go();
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
{ manageWells(); }


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
    ctio->ctxt.forread = true;
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
    Well::Data* wd = Well::MGR().get( mid );
    if ( !wd ) return false;

    if ( isdisppropopened_ == false )
    {
	uiwellpropdlg_ = new uiWellDispPropDlg( parent(), wd );
	uiwellpropdlg_->applyAllReq.notify(
			    mCB(this,uiWellPartServer,applyAll) );
	uiwellpropdlg_->windowClosed.notify(
			    mCB(this,uiWellPartServer, wellPropDlgClosed) );
	isdisppropopened_ = uiwellpropdlg_->go();
    }
    return true;
}


void uiWellPartServer::wellPropDlgClosed( CallBacker* cb)
{
    isdisppropopened_ = false;
    mDynamicCastGet(uiWellDispPropDlg*,dlg,cb)
    if ( !dlg ) { pErrMsg("Huh"); return; }
    const Well::Data* edwd = dlg->wellData();
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
	uiMSG().error( "Could not write display properties for \n", w.name() );
}


void uiWellPartServer::applyAll( CallBacker* cb )
{
    mDynamicCastGet(uiWellDispPropDlg*,dlg,cb)
    if ( !dlg ) { pErrMsg("Huh"); return; }
    const Well::Data* edwd = dlg->wellData();
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
    Well::Data* wd = Well::MGR().get( mid );
    if ( !wd ) return;

    uiWellDisplayWin* welldispwin = new uiWellDisplayWin( parent(), *wd );
    welldispwin->show();
}


bool uiWellPartServer::hasLogs( const MultiID& wellid ) const
{
    const Well::Data* wd = Well::MGR().get( wellid );
    return wd && wd->logs().size();
}


void uiWellPartServer::getLogNames( const MultiID& wellid,
					BufferStringSet& lognms ) const
{
    const Well::Data* wd = Well::MGR().get( wellid );
    if ( !wd ||  wd->logs().isEmpty() ) return;
    for ( int idx=0; idx<wd->logs().size(); idx++ )
	lognms.add( wd->logs().getLog(idx).name() );
}


void uiWellPartServer::manageWells()
{
    uiWellMan dlg( parent() );
    uiToolButton* tb = new uiToolButton( dlg.listGroup(), "multisimplewell",
					 tr("Create multiple simple wells"),
					 mCB(this,uiWellPartServer,simpImp) );
    dlg.addTool( tb );
    dlg.go();
}


class uiWellRockPhysLauncher : public uiDialog
{
public:

uiWellRockPhysLauncher( uiParent* p )
    : uiDialog( p, Setup("Rock Physics - Well Logs",
		"Select one or more wells to add well logs to",mTODOHelpKey) )
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
    uiSimpleMultiWellCreate dlg( parent() );
    if ( !dlg.go() )
	return;

    crwellids_ = dlg.createdWellIDs();
    if ( crwellids_.isEmpty() ) return;

    if ( dlg.wantDisplay() )
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

#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiWellPartServer::storeWell( const TypeSet<Coord3>& coords,
				  const char* wellname, MultiID& mid )
{
    if ( coords.isEmpty() )
	mErrRet(tr("Empty well track"))
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(Well);
    ctio->setObj(0); ctio->setName( wellname );
    if ( !ctio->fillObj() )
	mErrRet(tr("Cannot create an entry in the data store"))

    PtrMan<Well::Data> well = new Well::Data( wellname );
    Well::D2TModel* d2t = SI().zIsTime() ? new Well::D2TModel : 0;
    const float vel = mCast( float, d2t ? 3000 : 1 );
    const Coord3& c0( coords[0] );
    const float minz = (float) c0.z * vel;
    well->track().addPoint( c0, minz, minz );
    well->info().surfacecoord = Coord( c0.x, c0.y );
    if ( d2t ) d2t->add( minz, (float) c0.z );

    for ( int idx=1; idx<coords.size(); idx++ )
    {
	const Coord3& c( coords[idx] );
	well->track().addPoint( c, (float) c.z*vel );
	if ( d2t ) d2t->add( well->track().dah(idx), (float) c.z );
    }

    well->setD2TModel( d2t );
    Well::Writer wwr( *ctio->ioobj, *well );
    if ( !wwr.put() )
	mErrRet( wwr.errMsg() )

    mid = ctio->ioobj->key();
    delete ctio->ioobj;
    return true;
}


bool uiWellPartServer::showAmplSpectrum( const MultiID& mid, const char* lognm )
{
    const Well::Data* wd = Well::MGR().get( mid );
    if ( !wd || wd->logs().isEmpty()  )
	return false;

    const Well::Log* log = wd->logs().getLog( lognm );
    if ( !log )
	mErrRet( tr("Cannot find log in well data."
                    "  Probably it has been deleted") )

    if ( !log->size() )
	mErrRet( tr("Well log is empty") )

    StepInterval<float> resamprg( log->dahRange() );
    TypeSet<float> resamplvals;	int resampsz = 0;
    if ( SI().zIsTime() && wd->haveD2TModel() )
    {
	const Well::D2TModel& d2t = *wd->d2TModel();
	resamprg.set(d2t.getTime(resamprg.start, wd->track()),
		     d2t.getTime(resamprg.stop, wd->track()),1);
	resamprg.step /= SI().zDomain().userFactor();
	resampsz = resamprg.nrSteps();
	for ( int idx=0; idx<resampsz; idx++ )
	{
	    const float dah = d2t.getDah( resamprg.atIndex( idx ),
					  wd->track() );
	    resamplvals += log->getValue( dah );
	}
    }
    else
    {
	resampsz = resamprg.nrSteps();
	resamprg.step = resamprg.width() / (float)log->size();
	for ( int idx=0; idx<resampsz; idx++ )
	    resamplvals += log->getValue( resamprg.atIndex( idx ) );
    }

    uiAmplSpectrum::Setup su( lognm, false,  resamprg.step );
    uiAmplSpectrum* asd = new uiAmplSpectrum( parent(), su );
    asd->setData( resamplvals.arr(), resampsz );
    asd->show();

    return true;
}


