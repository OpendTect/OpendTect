/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.cc,v 1.144 2003-04-17 15:18:33 nanne Exp $
________________________________________________________________________

-*/

#include "uivispartserv.h"

#include "attribslice.h"
#include "draw.h"
#include "errh.h"
#include "pickset.h"
#include "ptrman.h"
#include "settings.h"
#include "separstr.h"
#include "survinfo.h"
#include "visdataman.h"
#include "viscolortab.h"
#include "visgridsurf.h"
#include "vismaterial.h"
#include "visplanedatadisplay.h"
#include "visselman.h"
#include "vissurvinterpret.h"
#include "vissurvpickset.h"
#include "vissurvscene.h"
#include "vissurvsurf.h"
#include "vissurvwell.h"
#include "visvolumedisplay.h"
#include "visvolrender.h"
#include "vistexture3viewer.h"
#include "visrandomtrackdisplay.h"
#include "uiexecutor.h"
#include "uifiledlg.h"
#include "uigeninputdlg.h"
#include "uimaterialdlg.h"
#include "uimenu.h"
#include "uipickszdlg.h"
#include "uisellinest.h"
#include "uislicesel.h"
#include "uizscaledlg.h"
#include "uiworkareadlg.h"
#include "uicolor.h"
#include "uidset.h"
#include "uibinidtable.h"
#include "colortab.h"
#include "surfaceinfo.h"
#include "ioobj.h"
#include "ioman.h"


const int uiVisPartServer::evUpdateTree = 0;
const int uiVisPartServer::evSelection = 1;
const int uiVisPartServer::evDeSelection = 2;
const int uiVisPartServer::evGetNewCubeData = 3;
const int uiVisPartServer::evGetNewRandomPosData = 4;
const int uiVisPartServer::evMouseMove = 5;
const int uiVisPartServer::evGetRandomTracePosData = 6;
const int uiVisPartServer::evInteraction = 7;
const int uiVisPartServer::evSelectAttrib = 8;

const char* uiVisPartServer::appvelstr = "AppVel";
const char* uiVisPartServer::workareastr = "Work Area";


uiVisPartServer::uiVisPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , viewmode(false)
    , eventobjid(-1)
    , eventmutex(*new Threads::Mutex)
    , mouseposval( mUndefValue )
    , interpreterdisplay( 0 )
{
    visBase::DM().selMan().selnotifer.notify( 
	mCB(this,uiVisPartServer,selectObjCB) );
    visBase::DM().selMan().deselnotifer.notify( 
  	mCB(this,uiVisPartServer,deselectObjCB) );
}


uiVisPartServer::~uiVisPartServer()
{
    visBase::DM().selMan().selnotifer.remove(
	    mCB(this,uiVisPartServer,selectObjCB) );
    visBase::DM().selMan().deselnotifer.remove(
	    mCB(this,uiVisPartServer,deselectObjCB) );
    delete &eventmutex;
}


const char* uiVisPartServer::name() const  { return "Visualisation"; }


void uiVisPartServer::setObjectName( int id, const char* nm )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    if ( obj ) obj->setName( nm );
}


const char* uiVisPartServer::getObjectName( int id ) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    if ( !obj ) return 0;
    return obj->name();
}


int uiVisPartServer::addScene()
{
    visSurvey::Scene* newscene = visSurvey::Scene::create();
    newscene->mouseposchange.notify( mCB(this,uiVisPartServer,mouseMoveCB) );
    newscene->ref();
    scenes += newscene;
    return newscene->id();
}


void uiVisPartServer::removeScene( int sceneid )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( scene )
    {
	scene->mouseposchange.remove( mCB(this,uiVisPartServer,mouseMoveCB) );
	scene->unRef();
	scenes -= scene;
	return;
    }
}


void uiVisPartServer::shareObject( int sceneid, int id )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return;

    mDynamicCastGet(visBase::SceneObject*, so, visBase::DM().getObj( id ) )
    if ( !so ) return;

    scene->addObject( so );
    sendEvent( evUpdateTree );
}


int uiVisPartServer::addInlCrlTsl( int sceneid, int type )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return -1;

    visSurvey::PlaneDataDisplay* pdd = visSurvey::PlaneDataDisplay::create();
    visSurvey::PlaneDataDisplay::Type pddtype =
	!type ? visSurvey::PlaneDataDisplay::Inline
	      : ( type == 1 ? visSurvey::PlaneDataDisplay::Crossline
		            : visSurvey::PlaneDataDisplay::Timeslice );
    pdd->setType( pddtype );
    scene->addObject( pdd );

    setUpConnections( pdd->id() );
    return pdd->id();
}


int uiVisPartServer::addVolView( int sceneid )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return -1;

    visSurvey::VolumeDisplay* vd = visSurvey::VolumeDisplay::create();
    scene->addObject( vd );

    setUpConnections( vd->id() );
    return vd->id();
}


int uiVisPartServer::addRandomLine( int sceneid )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return -1;

    visSurvey::RandomTrackDisplay* rtd = 
			visSurvey::RandomTrackDisplay::create();
    scene->addObject( rtd );

    setUpConnections( rtd->id() );
    return rtd->id();
}


int uiVisPartServer::addSurface( int sceneid, const MultiID& emhorid )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return -1;

    visSurvey::SurfaceDisplay* sd = visSurvey::SurfaceDisplay::create();
    sd->setTransformation( visSurvey::SPM().getUTM2DisplayTransform() );

    PtrMan<Executor> exec = sd->setSurfaceId( emhorid );
    if ( !exec )
    {
	sd->ref(); sd->unRef();
	pErrMsg( "EarthModel em does not exist" );
	return -1;
    }

    uiExecutor uiexec (appserv().parent(), *exec );
    if ( !uiexec.execute() )
    {
	sd->ref(); sd->unRef();
	return -1;
    }

    sd->setZValues();
    scene->addObject( sd ); //Must be done before loading surface
    				 //Otherwise the transform won't be set


    setUpConnections( sd->id() );
    return sd->id();
}


void uiVisPartServer::getSurfaceInfo( ObjectSet<SurfaceInfo>& hinfos )
{
    TypeSet<int> sceneids;
    getChildIds( -1, sceneids );
    for ( int ids=0; ids<sceneids.size(); ids++ )
    {
	TypeSet<int> visids;
	getChildIds( sceneids[ids], visids );
	for ( int idv=0; idv<visids.size(); idv++ )
	{
	    int visid = visids[idv];
	    if ( isHorizon( visid ) )
	    {
		BufferString attrnm( "" );
		const AttribSelSpec* as = getSelSpec( visid );
		if ( as ) attrnm = as->userRef();
		hinfos += new SurfaceInfo( getObjectName(visid), "", visid,
		       attrnm );
	    }
	}
    }
}


int uiVisPartServer::addWell( int sceneid, const MultiID& emwellid )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return -1;

    visSurvey::WellDisplay* wd = visSurvey::WellDisplay::create();
    if ( !wd->setWellId( emwellid ) )
    {
	wd->ref(); wd->unRef();
	pErrMsg( "EarthModel em does not exist" );
	return -1;
    }

    if ( wd->depthIsT() )
	scene->addObject( wd );
    else
	scene->addObject( wd );

    setUpConnections( wd->id() );
    return wd->id();
}


int uiVisPartServer::addPickSet(int sceneid, const PickSet& pickset )
{
    if ( sceneid < 0 )
    {
	TypeSet<int> sceneids;
	getChildIds( -1, sceneids );
	if ( sceneids.size() ) sceneid = sceneids[0];
    }
	    
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return -1;

    visSurvey::PickSetDisplay* psd = visSurvey::PickSetDisplay::create();

    psd->setColor( pickset.color );
    psd->setName( pickset.name() );
    setPickSetData( psd->id(), pickset );

    scene->addObject( psd );
    setUpConnections( psd->id() );
    return psd->id();
}


void uiVisPartServer::setPickSetData( int id, const PickSet& pickset )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,dobj)
    if ( !psd ) return;

    psd->removeAll();
    int nrpicks = pickset.size();
    for ( int idx=0; idx<nrpicks; idx++ )
    {
	Coord crd( pickset[idx].pos );
	psd->addPick( Coord3(crd.x,crd.y,pickset[idx].z) );
    }
}


void uiVisPartServer::getAllPickSets( UserIDSet& pset )
{
    TypeSet<int> sceneids;
    getChildIds( -1, sceneids );
    for ( int ids=0; ids<sceneids.size(); ids++ )
    {
	TypeSet<int> visids;
	getChildIds( sceneids[ids], visids );
	for ( int idv=0; idv<visids.size(); idv++ )
	{
	    if ( isPickSet( visids[idv] ) )
		pset.add( getObjectName( visids[idv] ) );
	}
    }
}


void uiVisPartServer::getPickSetData( int id, PickSet& pickset ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::PickSetDisplay*,psd,dobj)
    if ( !psd ) return;

    pickset.color = psd->getMaterial()->getColor();
    pickset.color.setTransparency( 0 );
    for ( int idx=0; idx<psd->nrPicks(); idx++ )
    {
	Coord3 pos = psd->getPick( idx );
	pickset += PickLocation( pos.x, pos.y, pos.z );
    }
}


MultiID uiVisPartServer::getMultiID( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::SurfaceDisplay*,sd,dobj)
    if ( sd ) return sd->surfaceId();

    mDynamicCastGet(const visSurvey::WellDisplay*,wd,dobj)
    if ( wd ) return wd->wellId();

    return MultiID();
}


int uiVisPartServer::getSelObjectId() const
{
    const TypeSet<int>& sel = visBase::DM().selMan().selected();
    return sel.size() ? sel[0] : -1;
}


void uiVisPartServer::setSelObjectId( int id )
{
    visBase::DM().selMan().select( id );
    if ( !viewmode )
	toggleDraggers();
}


#define mSelAttrib		1024
#define mPosition		1025
#define mProperties		1026
#define mRemove			1027
#define mShowAll		1028
#define mResolution		1029
#define mResolutionStart	1030
//Leave 100 spaces for the versions
#define mResolutionStop		1129
#define mSingleColor		1130
#define mColor			1131
#define mLineStyle		1132
#define mShowWellName		1133
#define mShare			1134
#define mShareStart		1135
////Leave 100 spaces for submnus
#define mShareStop		1235
#define mAnnotText		1236
#define mAnnotScale		1237
#define mSurveyBox		1238
#define mDumpIV			1240
#define mResetManip		1241
#define mSetPickSz		1242
#define mDuplicate		1243
#define mEditKnots		1244
#define mInsertKnot		1245
#define mInsertKnotStart	1246
////Leave 100 spaces for submnus
#define mInsertKnotStop		1346
#define mAddNode		1347
#define mRemoveNode		1348
#define mShiftHor		1349


void uiVisPartServer::makeSubMenu( uiPopupMenu& mnu, int sceneid, int id )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return;

    const visBase::DataObject* dobj = visBase::DM().getObj( id );

    if ( hasAttrib( id ) )
	 mnu.insertItem( new uiMenuItem("Select attribute ..."), mSelAttrib);
    if ( isMovable( id ) )
	mnu.insertItem( new uiMenuItem("Position ..."), mPosition );
    if ( isManipulated( id ) )
	mnu.insertItem( new uiMenuItem("Reset manipulation"), mResetManip );
    if ( hasColor( id ) )
	mnu.insertItem( new uiMenuItem("Color ..."), mColor );
    if ( hasMaterial( id ) )
	mnu.insertItem( new uiMenuItem("Properties ..."), mProperties );
    if ( hasDuplicate( id ) )
	mnu.insertItem( new uiMenuItem("Duplicate"), mDuplicate );

    if ( sceneid == id )
    {
	const bool showcube = scene->isAnnotShown();

	uiMenuItem* anntxt = new uiMenuItem("Annotation text");
	mnu.insertItem( anntxt, mAnnotText );
	anntxt->setChecked( showcube && scene->isAnnotTextShown() );
	anntxt->setEnabled( showcube );

	uiMenuItem* annscale = new uiMenuItem("Annotation scale");
	mnu.insertItem( annscale, mAnnotScale );
	annscale->setChecked( showcube && scene->isAnnotScaleShown() );
	annscale->setEnabled( showcube );

	uiMenuItem* annsurv = new uiMenuItem("Survey box");
	mnu.insertItem( annsurv, mSurveyBox );
	annsurv->setChecked( showcube );

	bool doi = false;
	Settings::common().getYN( IOPar::compKey("dTect","Dump OI Menu"), doi );
	if ( doi )
	    mnu.insertItem( new uiMenuItem("Dump OI ..."), mDumpIV );
    }

    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dobj)
    if ( pdd )
    {
	uiPopupMenu* resmnu = new uiPopupMenu( appserv().parent(),"Resolution");
	const int curres = pdd->getResolution();
	const int nrres = pdd->getNrResolutions();
	for ( int idx=0; idx<nrres; idx++ )
	{
	    uiMenuItem* resitm = new uiMenuItem(pdd->getResName(idx));
	    resmnu->insertItem( resitm, mResolutionStart+idx );
	    resitm->setChecked( curres==idx );
	}
	mnu.insertItem( resmnu );
    }

    mDynamicCastGet(const visSurvey::PickSetDisplay*,psd,dobj)
    if ( psd )
    {
	uiMenuItem* showitm = new uiMenuItem("Show all");
	mnu.insertItem( showitm, mShowAll );
	showitm->setChecked( psd->allShown() );

	mnu.insertItem( new uiMenuItem("Properties ..."), mSetPickSz );
    }

    mDynamicCastGet(const visSurvey::SurfaceDisplay*,sd,dobj)
    if ( sd )
    {
	uiMenuItem* colitm = new uiMenuItem("Use single color");
	mnu.insertItem( colitm, mSingleColor );
	colitm->setChecked( !sd->usesTexture() );

	uiPopupMenu* resmnu = new uiPopupMenu( appserv().parent(),"Resolution");
	const int curres = sd->getResolution();
	const int nrres = sd->getNrResolutions();
	for ( int idx=0; idx<nrres; idx++ )
	{
	    colitm = new uiMenuItem(sd->getResName(idx));
	    resmnu->insertItem( colitm, mResolutionStart+idx );
	    colitm->setChecked( curres==idx );
	}
	
	mnu.insertItem( new uiMenuItem("Shift ..."), mShiftHor );
	mnu.insertItem( resmnu );
    }

    mDynamicCastGet(const visSurvey::WellDisplay*,wd,dobj)
    if ( wd )
    {
	mnu.insertItem( new uiMenuItem("Linestyle ..."), mLineStyle );
	uiMenuItem* welltxt = new uiMenuItem( "Show well name" );
	mnu.insertItem( welltxt, mShowWellName );
	welltxt->setChecked( wd->isWellTextShown() );
    }

    mDynamicCastGet(const visSurvey::RandomTrackDisplay*,rtd,dobj)
    if ( rtd )
    {
	mnu.insertItem( new uiMenuItem("Edit nodes"), mEditKnots );
	uiPopupMenu* insertknotmnu = new uiPopupMenu( appserv().parent(),
				    "Insert node before...");
	for ( int idx=0; idx<rtd->nrKnots(); idx++ )
	{
	    BufferString knotname = "node ";
	    knotname += idx;
	    uiMenuItem* itm = new uiMenuItem( knotname );
	    insertknotmnu->insertItem( itm, mInsertKnotStart+idx );
	}

	mnu.insertItem( insertknotmnu, mInsertKnot );
	
	uiPopupMenu* resmnu = new uiPopupMenu( appserv().parent(),"Resolution");
	const int curres = rtd->getResolution();
	const int nrres = rtd->getNrResolutions();
	for ( int idx=0; idx<nrres; idx++ )
	{
	    uiMenuItem* resitm = new uiMenuItem( rtd->getResName(idx) );
	    resmnu->insertItem( resitm, mResolutionStart+idx );
	    resitm->setChecked( curres==idx );
	}
	mnu.insertItem( resmnu );

    }

#ifdef __debug__
    if ( scenes.size() > 1 )
    {
	uiPopupMenu* sharemnu = new uiPopupMenu( appserv().parent(),
				    "Share with...");
	for ( int idx=0; idx<scenes.size(); idx++ )
	{
	    if ( scenes[idx]->getFirstIdx( id )==-1 )
	    {
		uiMenuItem* itm = new uiMenuItem(scenes[idx]->name());
		sharemnu->insertItem( itm, mShareStart+idx );
	    }
	}

	mnu.insertItem( sharemnu, mShare );
    }
#endif

    if ( scene->getFirstIdx(id) != -1 )
	mnu.insertItem( new uiMenuItem("Remove"), mRemove );

    return;
}


bool uiVisPartServer::handleSubMenuSel( int mnu, int sceneid, int id )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return false;

    bool rv = true, mustreturn = true;
    if ( mnu == mAnnotText )
	scene->showAnnotText( !scene->isAnnotTextShown() );
    else if ( mnu == mAnnotScale )
	scene->showAnnotScale( !scene->isAnnotScaleShown() );
    else if ( mnu == mSurveyBox )
	scene->showAnnot( !scene->isAnnotShown() );
    else if ( mnu == mDumpIV )
	rv = dumpOI( id );
    else if ( mnu == mDuplicate )
	rv = duplicateObject( id, sceneid );
    else if ( mnu == mSelAttrib )
	 rv = calculateAttrib(id,true);  //Will both select and calculate
    else if ( mnu == mPosition )
	rv = setPosition( id );
    else if ( mnu == mResetManip )
	rv = resetManipulation( id );
    else if ( mnu == mProperties )
	rv = setMaterial( id );
    else if ( mnu == mRemove )
	removeObject( id, sceneid );
    else
	mustreturn = false;

    if ( mustreturn )
	return rv;

    visBase::DataObject* dobj = visBase::DM().getObj( id );
    rv = true;

    if ( mnu == mShowAll )
    {
	mDynamicCastGet(visSurvey::PickSetDisplay*,psd,dobj)
	bool showall = !psd->allShown();
	psd->showAll( showall );
	if ( !showall )
	{
	    for ( int idx=0; idx<scenes.size(); idx++ )
		scenes[idx]->filterPicks();
	}
    }

    else if ( mnu == mEditKnots )
    {
	mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,dobj)
	TypeSet<BinID> bidset;
	rtd->getAllKnotPos( bidset );
	uiBinIDTableDlg dlg( appserv().parent(), "Specify nodes", bidset );
	if ( dlg.go() )
	{
	    bool viewmodeswap = false;
	    if ( viewmode ) { setViewMode( false ); viewmodeswap = true; }
	    TypeSet<BinID> newbids;
	    dlg.getBinIDs( newbids );
	    if ( newbids.size() < 2 ) return true;
	    int nrknots = rtd->nrKnots();
	    while ( nrknots > newbids.size() )
	    {
		rtd->removeKnot( nrknots-1 );
		nrknots--;
	    }
		
	    for ( int idx=0; idx<newbids.size(); idx++ )
	    {
		const BinID bid = newbids[idx];
		if ( idx < nrknots )
		    rtd->setKnotPos( idx, bid );
		else
		    rtd->addKnot( bid );
	    }

	    setSelObjectId( rtd->id() );
	    calculateAttrib( rtd->id(), false );
	    if ( viewmodeswap ) setViewMode( true );
	}
    }

    else if ( mnu >= mInsertKnotStart && mnu <= mInsertKnotStop )
    {
	mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,dobj)
	int knotidx = mnu-mInsertKnotStart;

	BinID newknot;
	if ( !knotidx )
	{
	    const BinID lastknot = rtd->getKnotPos( 0 );
	    const BinID secondlastknot = rtd->getKnotPos( 1 );
	    const BinID diff = lastknot-secondlastknot;
	    newknot = lastknot+diff;
	}
	else
	{
	    const BinID previousknot = rtd->getKnotPos( knotidx-1 );
	    const BinID nextknot = rtd->getKnotPos( knotidx );
	    newknot = BinID((nextknot.inl+previousknot.inl)/2,
		    	    (nextknot.crl+previousknot.crl)/2 );
	}

	rtd->insertKnot( knotidx, newknot );
	setSelObjectId( rtd->id() );
    }

    else if ( mnu >= mResolutionStart && mnu <= mResolutionStop )
    {
	const int resolution = mnu-mResolutionStart;
	mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,dobj)
	if ( sd ) sd->setResolution( resolution );
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,dobj)
	if ( pdd ) pdd->setResolution( resolution );
	mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,dobj)
	if ( rtd ) rtd->setResolution( resolution );
    }

    else if ( mnu == mSingleColor )
    {
	mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,dobj)
	sd->useTexture( !sd->usesTexture() );
    }

    else if ( mnu == mColor )
    {
	mDynamicCastGet(visBase::VisualObject*,vo,dobj)
	Color col = vo->getMaterial()->getColor();
	if ( select( col, appserv().parent(), "Color selection", false ) )
	    vo->getMaterial()->setColor( col );
    }

    else if ( mnu == mLineStyle )
    {
	mDynamicCastGet(visSurvey::WellDisplay*,wd,dobj)
	LineStyleDlg dlg( appserv().parent(), wd->lineStyle(), 0, false, true );
	if ( dlg.go() )
	    wd->setLineStyle( dlg.getLineStyle() );
    }

    else if ( mnu == mShowWellName )
    {
	mDynamicCastGet(visSurvey::WellDisplay*,wd,dobj)
	wd->showWellText( !wd->isWellTextShown() );
    }

    else if ( mnu >= mShareStart && mnu <= mShareStop )
    {
	const int sceneidx = mnu-mShareStart;
	mDynamicCastGet(visBase::SceneObject*,sceneobj,dobj)
	if ( !sceneobj ) { pErrMsg( "Huh" ); return false; }
	shareObject( scenes[sceneidx]->id(), sceneobj->id() );
    }

    else if ( mnu == mSetPickSz )
    {
	mDynamicCastGet(visSurvey::PickSetDisplay*,psd,dobj)
	uiPickSizeDlg dlg( appserv().parent(), psd );
	dlg.go();
    }

    else if ( mnu == mShiftHor )
    {
	mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,dobj)
	float shift = sd->getTimeShift();
	DataInpSpec* inpspec = new FloatInpSpec( shift );
	uiGenInputDlg dlg( appserv().parent(), "Specify horizon shift", 
			   "Shift (ms)", inpspec );
	if ( dlg.go() )
	{
	    sd->setTimeShift( dlg.getfValue() );
	    calculateAttrib( sd->id(), false );
	}

	sendEvent( evInteraction );
    }

    return rv;
}


void uiVisPartServer::getChildIds( int id, TypeSet<int>& childids ) const
{
    childids.erase();

    if ( id==-1 )
    {
	for ( int idx=0; idx<scenes.size(); idx++ )
	    childids += scenes[idx]->id();

	return;
    }

    const visSurvey::Scene* scene = getScene( id );
    if ( scene )
    {
	for ( int idx=0; idx<scene->size(); idx++ )
	{
	    childids += scene->getObject( idx )->id();
	}

	return;
    }

    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,dobj)
    if ( vd )
    {
	int inl, crl, tsl;
	vd->getPlaneIds( inl, crl, tsl );
	childids += inl;
	childids += crl;
	childids += tsl;
	childids += vd->getVolRenId();
    }
}


BufferString uiVisPartServer::getTreeInfo( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );

    BufferString res;
    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dobj)
    if ( pdd )
    {
	const visSurvey::PlaneDataDisplay::Type type = pdd->getType();
	if ( type==visSurvey::PlaneDataDisplay::Inline )
	    res = pdd->getCubeSampling(true).hrg.start.inl;
	else if ( type==visSurvey::PlaneDataDisplay::Crossline )
	    res = pdd->getCubeSampling(true).hrg.start.crl;
	else
	{
	    float val = pdd->getCubeSampling(true).zrg.start;
	    res = SI().zIsTime() ? mNINT(val * 1000) : val;
	}
    }

    mDynamicCastGet(const visSurvey::PickSetDisplay*,psd,dobj)
    if ( psd )
        res = psd->nrPicks();

    mDynamicCastGet(const visSurvey::SurfaceDisplay*,sd,dobj)
    if ( sd )
	res = sd->getTimeShift();

    mDynamicCastGet(const visBase::MovableTextureSlice*,mts,dobj)
    if ( mts )
    {
	TypeSet<int> volids;
	visBase::DM().getIds( typeid(visSurvey::VolumeDisplay), volids );
	for ( int idx=0; idx<volids.size(); idx++ )
	{
	    visBase::DataObject* obj = visBase::DM().getObj( volids[idx] );
	    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
	    if ( !vd ) continue;
	    int inlid, crlid, tslid;
	    vd->getPlaneIds( inlid, crlid, tslid );
	    if ( id == inlid || id == crlid )
		res += vd->getPlanePos( id );
	    else if ( id == tslid )
	    {
		float val = vd->getPlanePos( id );
		res += SI().zIsTime() ? mNINT(val * 1000) : val;
	    }
	}
    }

    return res;
}



BufferString uiVisPartServer::getDisplayName( int id ) const
{
    const AttribSelSpec* as = getSelSpec( id );
    BufferString dispname( as ? as->userRef() : 0 );
    if ( as && as->isNN() )
    {
	dispname = as->objectRef();
	const char* nodenm = as->userRef();
	if ( IOObj::isKey(as->userRef()) )
	    nodenm = IOM().nameOf( as->userRef(), false );
	dispname += " ("; dispname += nodenm; dispname += ")";
    }

    if ( isHorizon(id) || isFault(id) )
    {
	bool hasattrnm = dispname[0];
	if ( hasattrnm ) dispname += " (";
	dispname += getObjectName( id );
	if ( hasattrnm ) dispname += ")";
    }
    else if ( as && !dispname[0] )
	dispname = "<right-click>";
    else if ( !as )
	dispname = getObjectName( id );

    return dispname;
}


bool uiVisPartServer::isInlCrlTsl( int id, int type ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dobj)
    visSurvey::PlaneDataDisplay::Type pddtype =
	!type ? visSurvey::PlaneDataDisplay::Inline
	      : ( type == 1 ? visSurvey::PlaneDataDisplay::Crossline
		            : visSurvey::PlaneDataDisplay::Timeslice );
    return pdd && pdd->getType() == pddtype;
}


bool uiVisPartServer::isVolView( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::VolumeDisplay*,vd,dobj)
    return vd;
}


bool uiVisPartServer::isRandomLine( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::RandomTrackDisplay*,rtd,dobj)
    return rtd;
}


bool uiVisPartServer::isPickSet( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::PickSetDisplay*,psd,dobj)
    return psd;
}


bool uiVisPartServer::isWell( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::WellDisplay*,wd,dobj)
    return wd;
}


bool uiVisPartServer::isHorizon( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::SurfaceDisplay*,sd,dobj)
    return sd ? sd->isHorizon() : false;
}


bool uiVisPartServer::isFault( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::SurfaceDisplay*,sd,dobj)
    return sd ? !sd->isHorizon() : false;
}


const CubeSampling* uiVisPartServer::getCubeSampling( int id ) const
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,dobj)
    if ( pdd ) return &pdd->getCubeSampling(true);

    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,dobj)
    if ( vd ) return &vd->getCubeSampling();

    mDynamicCastGet(visSurvey::SurfaceInterpreterDisplay*,si,dobj)
    if ( si ) return &si->getCubeSampling();

    return 0;
}


const AttribSliceSet* uiVisPartServer::getCachedData( int id ) const
{
    const visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,obj)
    mDynamicCastGet(const visSurvey::VolumeDisplay*,vd,obj)
    if ( pdd ) return pdd->getPrevData();
    if ( vd ) return vd->getPrevData();
    else return 0;
}


bool uiVisPartServer::setCubeData( int id, AttribSliceSet* sliceset )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    if ( pdd ) return pdd->putNewData(sliceset);
    if ( vd ) return vd->putNewData( sliceset );

    delete sliceset;
    return false;
}

/*
void uiVisPartServer::setSliceIdx( int id, int idx )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( pdd ) pdd->setDataIdx( idx );
}
*/

void uiVisPartServer::getRandomPosDataPos( int id,
			   ObjectSet< TypeSet<BinIDZValue> >& bidzvset) const
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,dobj)
    if ( sd )
	sd->getAttribPositions( bidzvset, true, 0 );
}


void uiVisPartServer::setRandomPosData( int id,
		    const ObjectSet<const TypeSet<const BinIDZValue> >& nd )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,dobj)
    if ( sd ) sd->putNewData( nd );
}


void uiVisPartServer::getRandomTrackPositions( int id, 
						TypeSet<BinID>& bids ) const
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,dobj)
    if ( rtd ) rtd->getDataPositions( bids );
}
	

const Interval<float> uiVisPartServer::getRandomTraceZRange( int id ) const
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,dobj)
    return rtd ? rtd->getManipDepthInterval() : Interval<float>(0,0);
}


void uiVisPartServer::setRandomTrackData( int id, ObjectSet<SeisTrc>& data )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,dobj)
    if ( rtd ) rtd->putNewData( data );
}


Coord3 uiVisPartServer::getMousePos(bool xyt) const
{ return xyt ? xytmousepos : inlcrlmousepos; }


BufferString uiVisPartServer::getMousePosVal() const
{
    return mIsUndefined(mouseposval)
	? BufferString("") : BufferString(mouseposval);
}


void uiVisPartServer::setSelSpec( const AttribSelSpec& as )
{ attribspec = as; }


BufferString uiVisPartServer::getInteractionMsg( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );

    BufferString res;
    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dobj)
    if ( pdd )
    {
	const visSurvey::PlaneDataDisplay::Type type = pdd->getType();
	if ( type==visSurvey::PlaneDataDisplay::Inline )
	{
	    res = "Inline: ";
	    res += pdd->getCubeSampling(true).hrg.start.inl;
	}
	else if ( type==visSurvey::PlaneDataDisplay::Crossline )
	{
	    res = "Crossline: ";
	    res += pdd->getCubeSampling(true).hrg.start.crl;
	}
	else
	{
	    res = SI().zIsTime() ? "Time: " : "Depth: ";
	    float val = pdd->getCubeSampling(true).zrg.start;
	    res += SI().zIsTime() ? mNINT(val * 1000) : val;
	}
    }

    mDynamicCastGet(const visSurvey::PickSetDisplay*,psd,dobj)
    if ( psd )
    {
	res = "Nr of picks: ";
	res += psd->nrPicks();
    }

    mDynamicCastGet(const visSurvey::RandomTrackDisplay*,rtd,dobj)
    if ( rtd )
    {
	int knotidx = rtd->getSelKnotIdx();
	if ( knotidx >= 0 )
	{
	    BinID binid  = rtd->getManipKnotPos( knotidx );
	    res = "Node "; res += knotidx;
	    res += " Inl/Crl: ";
	    res += binid.inl; res += "/"; res += binid.crl;
	}
    }

    mDynamicCastGet(const visSurvey::SurfaceDisplay*,sd,dobj)
    if ( sd )
    {
	float shift = sd->getTimeShift();
	if ( shift )
	{
	    res = "Horizon shift: ";
	    res += shift; res += " (ms)";
	}
    }

    mDynamicCastGet(const visBase::MovableTextureSlice*,mts,dobj)
    if ( mts )
    {
	int dim = mts->dim();
	res = !dim ? "Inline: " : ( dim == 1 ? "Crossline: " : "Time: " );
	res += getTreeInfo( id );
    }

    return res;
}


int uiVisPartServer::getColTabId( int id ) const
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,dobj)
    if ( pdd ) return pdd->getColorTab().id();

    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,dobj)
    if ( vd ) return vd->getColorTab().id();

    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,dobj)
    if ( rtd ) return rtd->getColorTab().id();

    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,dobj)
    if ( sd ) return sd->getColorTab().id();

    return -1;
}


void uiVisPartServer::setClipRate( int id, float cr )
{
    visBase::DataObject* obj = visBase::DM().getObj( getColTabId(id) );
    mDynamicCastGet(visBase::VisColorTab*,coltab,obj)
    if ( coltab ) coltab->setClipRate( cr );
}


int uiVisPartServer::getEventObjId() const { return eventobjid; }


const AttribSelSpec* uiVisPartServer::getSelSpec( int id ) const
{
    const visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( pdd ) return &pdd->getAttribSelSpec();

    mDynamicCastGet(const visSurvey::SurfaceDisplay*,sd,obj)
    if ( sd ) return &sd->getAttribSelSpec();

    mDynamicCastGet(const visSurvey::VolumeDisplay*,vd,obj)
    if ( vd ) return &vd->getAttribSelSpec();

    mDynamicCastGet(const visSurvey::RandomTrackDisplay*,rtd,obj)
    if ( rtd ) return &rtd->getAttribSelSpec();

    return 0;
}


bool uiVisPartServer::deleteAllObjects()
{
    if ( interpreterdisplay ) interpreterdisplay->unRef();

    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	scenes[idx]->mouseposchange.remove(
	    			mCB(this,uiVisPartServer,mouseMoveCB) );
        scenes[idx]->unRef();
    }

    scenes.erase();

    return visBase::DM().reInit();
}


void uiVisPartServer::setViewMode(bool yn)
{
    if ( yn==viewmode ) return;
    viewmode = yn;
    toggleDraggers();
}


void uiVisPartServer::toggleDraggers()
{
    const TypeSet<int>& selected = visBase::DM().selMan().selected();

    for ( int sceneidx=0; sceneidx<scenes.size(); sceneidx++ )
    {
	visSurvey::Scene* scene = scenes[sceneidx];

	for ( int objidx=0; objidx<scene->size(); objidx++ )
	{
	    visBase::SceneObject* obj = scene->getObject( objidx );
	    bool isdraggeron = selected.indexOf(obj->id())!=-1 && !viewmode;

	    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
	    if ( pdd ) pdd->showDraggers(isdraggeron);
	    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
	    if ( vd ) vd->showBox(isdraggeron);
	    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,obj)
	    if ( rtd ) rtd->showDragger(isdraggeron);
	}
    }
}
    


void uiVisPartServer::setZScale()
{
    uiZScaleDlg dlg( appserv().parent() );
    dlg.go();
}


bool uiVisPartServer::setWorkingArea()
{
    uiWorkAreaDlg dlg( appserv().parent() );
    if ( !dlg.go() ) return false;

    TypeSet<int> sceneids;
    getChildIds( -1, sceneids );

    for ( int ids=0; ids<sceneids.size(); ids++ )
    {
	int sceneid = sceneids[ids];
	visBase::DataObject* obj = visBase::DM().getObj( sceneid );
	mDynamicCastGet(visSurvey::Scene*,scene,obj)
	if ( scene ) scene->updateRange();
	TypeSet<int> objids;
	getChildIds( sceneid, objids );
	for ( int ido=0; ido<objids.size(); ido++ )
	{
	    visBase::DataObject* dobj = visBase::DM().getObj( objids[ido] );
	    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,dobj)
	    if ( pdd ) pdd->setGeometry( true );
	}
    }

    return true;
}


bool uiVisPartServer::usePar( const IOPar& par )
{
    BufferString res;
    if ( par.get( workareastr, res ) )
    {
	FileMultiString fms(res);
	BinIDRange hrg; Interval<double> zrg;
	hrg.start.inl = atoi(fms[0]); hrg.stop.inl = atoi(fms[1]);
	hrg.start.crl = atoi(fms[2]); hrg.stop.crl = atoi(fms[3]);
	zrg.start = (double)atof(fms[4]); zrg.stop = (double)atof(fms[5]);
	const_cast<SurveyInfo&>(SI()).setWorkRange( hrg );
	const_cast<SurveyInfo&>(SI()).setWorkZRange( zrg );
    }

    if ( !visBase::DM().usePar( par ) )
	return false;

    TypeSet<int> sceneids;
    visBase::DM().getIds( typeid(visSurvey::Scene), sceneids );

    TypeSet<int> hasconnections;

    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	visSurvey::Scene* newscene =
	    	(visSurvey::Scene*) visBase::DM().getObj( sceneids[idx] );

	newscene->mouseposchange.notify(
				mCB( this, uiVisPartServer, mouseMoveCB ));
	scenes += newscene;
	newscene->ref();

	TypeSet<int> children;
	getChildIds( newscene->id(), children );

	for ( int idy=0; idy<children.size(); idy++ )
	{
	    calculateAttrib( children[idy], false );

	    if ( hasconnections.indexOf( children[idy] ) >= 0 ) continue;

	    setUpConnections( children[idy] );
	    hasconnections += children[idy];
	}
    }

    TypeSet<int> interpreters;
    visBase::DM().getIds(
	    typeid(visSurvey::SurfaceInterpreterDisplay), interpreters );
    interpreterdisplay = interpreters.size()
		?  dynamic_cast<visSurvey::SurfaceInterpreterDisplay*>(
		    visBase::DM().getObj( interpreters[0] ))
		: 0;

    if ( interpreterdisplay ) interpreterdisplay->ref();

    float appvel;
    if ( par.get( appvelstr, appvel ) )
	visSurvey::SPM().setZScale( appvel/1000 );

    return true;
}


void uiVisPartServer::fillPar( IOPar& par ) const
{
    TypeSet<int> storids;

    for ( int idx=0; idx<scenes.size(); idx++ )
	storids += scenes[idx]->id();

    par.set( appvelstr, visSurvey::SPM().getZScale()*1000 );

    BinIDRange hrg = SI().range();
    StepInterval<double> zrg = SI().zRange();
    FileMultiString fms;
    fms += hrg.start.inl; fms += hrg.stop.inl; fms += hrg.start.crl;
    fms += hrg.stop.crl; fms += zrg.start; fms += zrg.stop;
    par.set( workareastr, fms );

    visBase::DM().fillPar(par, storids);
}


void uiVisPartServer::turnOn( int id, bool yn )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VolRender*,vr,obj)
    if ( yn && vr && !vr->isInited() )
    {
	PtrMan<Executor> exec = vr->init();
	uiExecutor uiexec(appserv().parent(), *exec );
	uiexec.go();
    }

    mDynamicCastGet(visBase::VisualObject*,so,obj)
    if ( so ) so->turnOn( yn );

    TypeSet<int> sceneids;
    getChildIds( -1, sceneids );
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	visSurvey::Scene* scene =
		(visSurvey::Scene*) visBase::DM().getObj( sceneids[idx] );
	if ( scene ) scene->filterPicks();
    }
}


bool uiVisPartServer::isOn( int id ) const
{
    const visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet( const visBase::VisualObject*,so,obj)
    return so ? so->isOn() : false;
}

#define mGetScene( prepostfix ) \
prepostfix visSurvey::Scene* \
uiVisPartServer::getScene( int sceneid ) prepostfix \
{ \
    for ( int idx=0; idx<scenes.size(); idx++ ) \
    { \
	if ( scenes[idx]->id()==sceneid ) \
	{ \
	    return scenes[idx]; \
	} \
    } \
 \
    return 0; \
}

mGetScene( );
mGetScene( const ); 


void uiVisPartServer::removeObject( int id, int sceneid )
{
    visSurvey::Scene* scene = getScene( sceneid );
    int idx = scene->getFirstIdx( id );
    scene->removeObject( idx );

    Threads::MutexLocker lock( eventmutex );
    sendEvent( evUpdateTree );
}


bool uiVisPartServer::hasAttrib( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );

    mDynamicCastGet(const visSurvey::VolumeDisplay*,vd,dobj)
    if ( vd ) return true;

    mDynamicCastGet(const visSurvey::RandomTrackDisplay*,rtd,dobj)
    if ( rtd ) return true;

    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dobj)
    if ( pdd ) return true;

    mDynamicCastGet(const visSurvey::SurfaceDisplay*,sd,dobj)
    if ( sd && sd->usesTexture() ) return true;

    return false;
}
    

bool uiVisPartServer::selectAttrib( int id )
{
    eventmutex.lock();
    bool selected = sendEvent( evSelectAttrib );
    eventmutex.unlock();

    if ( !selected ) return false;

    const AttribSelSpec myattribspec( attribspec );
    setSelSpec( id, myattribspec );
    eventmutex.lock();
    eventobjid = id;
    sendEvent(evUpdateTree);
    eventmutex.unlock();

    return true;
}


void uiVisPartServer::setSelSpec( int id, const AttribSelSpec& myattribspec )
{

    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,dobj)
    if ( vd ) vd->setAttribSelSpec( myattribspec );

    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,dobj)
    if ( rtd ) rtd->setAttribSelSpec( myattribspec );

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,dobj)
    if ( pdd ) pdd->setAttribSelSpec( myattribspec );

    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,dobj)
    if ( sd ) sd->setAttribSelSpec( myattribspec );
}


bool uiVisPartServer::calculateAttrib( int id, bool newselect )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,dobj)
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,dobj)
    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,dobj)
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,dobj)

    const AttribSelSpec* as = getSelSpec( id );
    if ( !as ) return false;
    bool res = true;
    if ( newselect || ( as->id() < 0 && !sd ) )
	res = selectAttrib( id );
    else if ( sd && as->id() < 0 )
    {
	sd->setZValues();
	return true;
    }
	
    if ( !res ) return res;

    res = false;
    if ( vd || pdd )
    {
	Threads::MutexLocker lock( eventmutex );
	eventobjid = id;
	res = sendEvent( evGetNewCubeData );
    }

    if ( rtd )
    {
	Threads::MutexLocker lock( eventmutex );
	eventobjid = id;
	res = sendEvent( evGetRandomTracePosData );
    }

    if ( sd )
    {
	Threads::MutexLocker lock( eventmutex );
	eventobjid = id;
	res = sendEvent( evGetNewRandomPosData );
    }

    if ( res ) acceptManipulation( id );
    return res;
}



bool uiVisPartServer::isMovable( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );

    mDynamicCastGet(const visSurvey::VolumeDisplay*,vd,dobj)
    if ( vd ) return true;

    mDynamicCastGet(const visSurvey::RandomTrackDisplay*,rtd,dobj)
    if ( rtd ) return false;

    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dobj)
    if ( pdd ) return true;

    return false;
}


bool uiVisPartServer::setPosition( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    if ( !pdd && !vd ) return false;

    const CubeSampling initcs = pdd ? pdd->getCubeSampling( true )
				    : vd->getCubeSampling();
    uiSliceSel dlg( appserv().parent(), initcs,
		    mCB(this,uiVisPartServer,updatePlanePos), (bool)vd );
    if ( dlg.go() )
    {
	bool res;
	CubeSampling cs = dlg.getCubeSampling();
	if ( pdd )
	{
	    pdd->setCubeSampling( cs );
	    res = calculateAttrib( id, false );
	}
	else if ( vd )
	{
	    vd->setCubeSampling( cs );
	    res = calculateAttrib( id, false );
	}

	sendEvent( evInteraction );
	return res;
    }

    return true;
}


void uiVisPartServer::updatePlanePos( CallBacker* cb )
{
    int id = getSelObjectId();
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( !pdd ) return;
    mDynamicCastGet(uiSliceSel*,dlg,cb)
    if ( !dlg ) return;

    CubeSampling cs = dlg->getCubeSampling();
    pdd->setCubeSampling( cs );
    calculateAttrib( id, false );
    sendEvent( evInteraction );
}
  

bool uiVisPartServer::hasMaterial( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );

    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dobj)
    mDynamicCastGet(const visSurvey::VolumeDisplay*,vd,dobj)
    mDynamicCastGet(const visSurvey::RandomTrackDisplay*,rtd,dobj)
    mDynamicCastGet(const visSurvey::SurfaceDisplay*,sd,dobj)

    return ( pdd || vd || rtd || sd );
}


bool uiVisPartServer::setMaterial( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VisualObject*,vo,obj)
    if ( !vo ) return false;

    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,obj)
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,obj)
    if ( pdd || vd || rtd || (sd && sd->usesTexture()) )
    {
	uiMaterialDlg dlg( appserv().parent(), vo->getMaterial(), true,
	       		   true, false, false, false, true );
	dlg.go();
    }
    else
    {
	uiMaterialDlg dlg( appserv().parent(), vo->getMaterial(), true, 
			   true, false, false, false, true );
	dlg.go();
    }

    return true;
}


bool uiVisPartServer::hasColor( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );

    mDynamicCastGet(const visSurvey::WellDisplay*,wd,dobj)
    if ( wd ) return true;

    mDynamicCastGet(const visSurvey::SurfaceDisplay*,sd,dobj)
    if ( sd && !sd->usesTexture() ) return true;

    return false;
}


bool uiVisPartServer::hasDuplicate( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );

    mDynamicCastGet(const visSurvey::VolumeDisplay*,vd,dobj)
    if ( vd ) return true;

    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dobj)
    if ( pdd ) return true;

    return false;
}    


bool uiVisPartServer::duplicateObject( int id, int sceneid )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dobj)
    mDynamicCastGet(const visSurvey::VolumeDisplay*,vd,dobj)

    int newid = -1;
    if ( pdd )
    {
	newid = addInlCrlTsl( sceneid, 0 );
	visBase::DataObject* newobj = visBase::DM().getObj( newid );
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,newpdd,newobj)

	newpdd->setType( pdd->getType() );
	newpdd->setCubeSampling( pdd->getCubeSampling() );
	newpdd->setResolution( pdd->getResolution() );
	const char* ctnm = pdd->getColorTab().colorSeq().colors().name();
	newpdd->getColorTab().colorSeq().loadFromStorage( ctnm );
    }
    else if ( vd )
    {
	newid = addVolView( sceneid );
	visBase::DataObject* newobj = visBase::DM().getObj( newid );
	mDynamicCastGet(visSurvey::VolumeDisplay*,newvd,newobj)

	newvd->setCubeSampling( vd->getCubeSampling() );
	const char* ctnm = vd->getColorTab().colorSeq().colors().name();
	newvd->getColorTab().colorSeq().loadFromStorage( ctnm );
    }

    eventobjid = newid;
    setSelObjectId( newid );
    sendEvent( evUpdateTree );
    return true;
}


bool uiVisPartServer::dumpOI( int id ) const
{
    uiFileDialog filedlg( appserv().parent(), false, GetHomeDir(), "*.iv",
	    		  "Select output file" );
    if ( filedlg.go() )
    {
	visBase::DataObject* obj = visBase::DM().getObj( id );
	if ( !obj ) return false;
	return obj->dumpOIgraph( filedlg.fileName() );
    }

    return false;
}


bool uiVisPartServer::resetManipulation( int id )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,dobj)
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,dobj)
    if ( pdd ) pdd->resetManip();
    if ( vd ) vd->resetManip();
    return vd || pdd;
}


bool uiVisPartServer::isManipulated( int id ) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( pdd ) 
	return pdd->getCubeSampling( true )!=pdd->getCubeSampling( false );

    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    if ( vd )
	return vd->manipCenter()!=vd->center() || vd->manipWidth()!=vd->width();

    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,obj)
    if ( rtd )
	return rtd->isManipulated();

    return false;
}


void uiVisPartServer::acceptManipulation( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( pdd )
    {
	pdd->setCubeSampling( pdd->getCubeSampling(true));
	pdd->resetManip();
    }

    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    if ( vd )
    {
	vd->setCubeSampling( vd->getCubeSampling(true) );
	vd->resetManip();
    }

    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,obj)
    if ( rtd )
	rtd->acceptManip();
}


void uiVisPartServer::setUpConnections( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    if ( vd )
	vd->slicemoving.notify( mCB(this,uiVisPartServer,interactionCB) );

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( pdd )
	pdd->getMovementNotification()->notify( 
				    mCB(this,uiVisPartServer,interactionCB) );

    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,obj)
    if ( psd )
	psd->changed.notify( mCB(this,uiVisPartServer,interactionCB) );

    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,obj)
    if ( rtd )
    {
	rtd->knotmoving.notify( mCB(this,uiVisPartServer,interactionCB) );
	rtd->rightclick.notify( mCB(this,uiVisPartServer,createPopupMenu) );
    }

}


int uiVisPartServer::addSurfTrackerCube( int sceneid )
{
    if ( !interpreterdisplay )
    {
	interpreterdisplay = visSurvey::SurfaceInterpreterDisplay::create();
	interpreterdisplay->ref();
    }

    visSurvey::Scene* scene = getScene( sceneid );
    if ( scene->getFirstIdx(interpreterdisplay) == -1 )
	scene->addObject( interpreterdisplay );

    return interpreterdisplay->id();
}


int uiVisPartServer::getSurfTrackerCubeId()
{
    return interpreterdisplay ? interpreterdisplay->id() : 0;
}


int uiVisPartServer::addSurfEditor( int sceneid, Geometry::GridSurface& surf )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return -1;


    visBase::EditableGridSurface* surfed=visBase::EditableGridSurface::create();
    surfed->setTransformation( visSurvey::SPM().getUTM2DisplayTransform() );
    PtrMan<Executor> exec = surfed->setSurface( surf );
    uiExecutor uiexec(appserv().parent(), *exec );
    if ( !uiexec.execute() )
	return -1;

    scene->addObject( surfed );

    setUpConnections( surfed->id() );
    return surfed->id();
}


void uiVisPartServer::selectObjCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sel,cb);
    if ( !viewmode )
	toggleDraggers();

    Threads::MutexLocker lock( eventmutex );
    eventobjid = sel;
    sendEvent( evSelection );
}


void uiVisPartServer::deselectObjCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,oldsel,cb);
    if ( isManipulated( oldsel ) && hasAttrib(oldsel) )
	calculateAttrib( oldsel, false );

    if ( !viewmode )
	toggleDraggers();


    Threads::MutexLocker lock( eventmutex );
    eventobjid = oldsel;
    sendEvent( evDeSelection );
}


void uiVisPartServer::interactionCB(CallBacker*)
{
    Threads::MutexLocker lock( eventmutex );
    eventobjid = getSelObjectId();
    sendEvent( evInteraction );
}


void uiVisPartServer::mouseMoveCB( CallBacker* cb )
{
    mDynamicCastGet(visSurvey::Scene*,scene,cb)
    if ( !cb ) return;

    int selid = getSelObjectId();
    const visBase::DataObject* dobj = visBase::DM().getObj( selid );
    mDynamicCastGet(const visSurvey::PickSetDisplay*,psd,dobj)

    if ( selid==-1 || psd )
    {
	Threads::MutexLocker lock( eventmutex );
	xytmousepos = scene->getMousePos(true);
	inlcrlmousepos = scene->getMousePos(false);
	mouseposval = scene->getMousePosValue();
	sendEvent( evMouseMove );
    }
}


void uiVisPartServer::createPopupMenu( CallBacker* cb )
{
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,cb)

    int objid;
    uiPopupMenu mnu( appserv().parent(), "Popup" );    
    if ( rtd )
    {
	mnu.insertItem( new uiMenuItem("Insert node"), mAddNode );
	if ( rtd->nrKnots() > 2 )
	    mnu.insertItem( new uiMenuItem("Remove nearest node"), mRemoveNode);
	objid = rtd->id();
    }

    const int mnuid = mnu.nrItems() ? mnu.exec() : -1;
    handlePopupMenu( mnuid, objid );
}


void uiVisPartServer::handlePopupMenu( int mnuid, int objid )
{
    visBase::DataObject* dobj = visBase::DM().getObj( objid );
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,dobj)

    if ( rtd )
    {
	int sectidx = rtd->getSectionIdx();
	BinID pos = rtd->getClickedPos();
	if ( mnuid == mAddNode )
	    rtd->insertKnot( sectidx+1, pos );
	else if ( mnuid == mRemoveNode )
	    rtd->removeNearestKnot( sectidx, pos );
    }
}
