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
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uisimplemultiwell.h"
#include "uitoolbutton.h"
#include "uiwellrdmlinedlg.h"
#include "uiwelldisplay.h"
#include "uiwelldisppropdlg.h"
#include "uiwelldlgs.h"
#include "uiwellimpasc.h"
#include "uiwelllogtools.h"
#include "uiwellman.h"

#include "arrayndimpl.h"
#include "color.h"
#include "ctxtioobj.h"
#include "errh.h"
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
    , uiwellpropdlg_(0)		    
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


bool uiWellPartServer::bulkImportTrack()
{
    uiBulkTrackImport dlg( parent() );
    return dlg.go();
}


bool uiWellPartServer::bulkImportLogs()
{
    uiBulkLogImport dlg( parent() );
    return dlg.go();
}

bool uiWellPartServer::bulkImportMarkers()
{
    uiBulkMarkerImport dlg( parent() );
    return dlg.go();
}


bool uiWellPartServer::importTrack()
{
    uiWellImportAsc dlg( parent() );
    return dlg.go();
}


bool uiWellPartServer::importLogs()
{
    manageWells(); return true;
}


bool uiWellPartServer::importMarkers()
{
    manageWells(); return true;
}


bool uiWellPartServer::selectWells( ObjectSet<MultiID>& wellids )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(Well);
    ctio->ctxt.forread = true;
    uiIOObjSelDlg dlg( parent(), *ctio, 0, true );
    if ( !dlg.go() ) return false;

    deepErase( wellids );
    const int nrsel = dlg.nrSel();
    for ( int idx=0; idx<nrsel; idx++ )
	wellids += new MultiID( dlg.selected(idx) );

    return wellids.size();
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
    Well::Writer wr( Well::IO::getMainFileName(key), w );
    if ( !wr.putDispProps() )
    uiMSG().error( "Could not write display properties for \n",
    w.name() );
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
					 "Create multiple simple wells",
					 mCB(this,uiWellPartServer,simpImp) );
    dlg.addTool( tb );
    dlg.go();
}


void uiWellPartServer::simpImp( CallBacker* )
{ createSimpleWells(); }


void uiWellPartServer::createSimpleWells()
{
    uiSimpleMultiWellCreate dlg( parent() );
    if ( !dlg.go() )
	return;

    crwellids_ = dlg.createdWellIDs();
    if ( dlg.wantDisplay() )
	sendEvent( evDisplayWell() );
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
    rdmlinedlg_->windowClosed.notify(mCB(this,uiWellPartServer,rdmlnDlgClosed));
    rdmlinedlg_->go();
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
	mErrRet("Empty well track")
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(Well);
    ctio->setObj(0); ctio->setName( wellname );
    if ( !ctio->fillObj() )
	mErrRet("Cannot create an entry in the data store")
    PtrMan<Translator> tr = ctio->ioobj->createTranslator();
    mDynamicCastGet(WellTranslator*,wtr,tr.ptr())
    if ( !wtr ) mErrRet( "Please choose a different name for the well.\n"
			 "Another type object with this name already exists." );

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
    if ( !wtr->write(*well,*ctio->ioobj) )
	mErrRet( "Cannot write well. Please check permissions." )

    mid = ctio->ioobj->key();
    return true;
}


bool uiWellPartServer::showAmplSpectrum( const MultiID& mid, const char* lognm )
{
    const Well::Data* wd = Well::MGR().get( mid );
    if ( !wd || wd->logs().isEmpty()  ) 
	return false;

    const Well::Log* log = wd->logs().getLog( lognm );
    if ( !log )
	mErrRet( "Cannot find log in well data. Probably it has been deleted" )

    if ( !log->size() )
	mErrRet( "Well log is empty" )

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


