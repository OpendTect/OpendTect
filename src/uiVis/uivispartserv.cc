/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.cc,v 1.229 2004-07-30 14:29:26 kristofer Exp $
________________________________________________________________________

-*/

#include "uivispartserv.h"

#include "attribslice.h"
#include "separstr.h"
#include "survinfo.h"
#include "visdataman.h"
#include "viscolortab.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "visselman.h"
#include "vissurvscene.h"
#include "uifiledlg.h"
#include "uimaterialdlg.h"
#include "uizscaledlg.h"
#include "uiworkareadlg.h"
#include "uimenu.h"
#include "uicursor.h"
#include "iopar.h"
#include "binidvalset.h"
#include "uivismenu.h"
#include "uicolor.h"
#include "uitrackingman.h"
#include "visinterpret.h"
#include "seisbuf.h"


const int uiVisPartServer::evUpdateTree =	0;
const int uiVisPartServer::evSelection =	1;
const int uiVisPartServer::evDeSelection =	2;
const int uiVisPartServer::evGetNewData =	3;
const int uiVisPartServer::evMouseMove =	4;
const int uiVisPartServer::evInteraction =	5;
const int uiVisPartServer::evSelectAttrib =	6;
const int uiVisPartServer::evSelectColorAttrib=	7;
const int uiVisPartServer::evGetColorData =	8;
const int uiVisPartServer::evViewAll =		9;
const int uiVisPartServer::evToHomePos =	10;

const char* uiVisPartServer::appvelstr = "AppVel";
const char* uiVisPartServer::workareastr = "Work Area";


uiVisPartServer::uiVisPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , viewmode(false)
    , eventobjid(-1)
    , eventmutex(*new Threads::Mutex)
    , mouseposval( mUndefValue )
    , trackdlg(0)
{
    visBase::DM().selMan().selnotifer.notify( 
	mCB(this,uiVisPartServer,selectObjCB) );
    visBase::DM().selMan().deselnotifer.notify( 
	mCB(this,uiVisPartServer,deselectObjCB) );
}


void uiVisPartServer::unlockEvent()
{ eventmutex.unlock(); }


uiVisPartServer::~uiVisPartServer()
{
    visBase::DM().selMan().selnotifer.remove(
	    mCB(this,uiVisPartServer,selectObjCB) );
    visBase::DM().selMan().deselnotifer.remove(
	    mCB(this,uiVisPartServer,deselectObjCB) );
    delete &eventmutex;

    for ( int idx=0; idx<menus.size(); idx++ )
	menus[idx]->unRef();

    if ( trackdlg )
    {
	trackdlg->close();
	delete trackdlg;
    }
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


int uiVisPartServer::addScene(visSurvey::Scene* newscene)
{
    if ( !newscene ) newscene = visSurvey::Scene::create();
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


bool uiVisPartServer::showMenu( int id, int menutype, const TypeSet<int>* path)
{
    uiVisMenu* menu = getMenu( id, false );
    return menu ? menu->executeMenu(menutype,path) : true;
}


uiVisMenu* uiVisPartServer::getMenu(int visid, bool create)
{
    for ( int idx=0; idx<menus.size(); idx++ )
    {
	if ( menus[idx]->id()==visid )
	    return menus[idx];
    }
    
    if ( !create ) return 0;

    uiVisMenu* res = new uiVisMenu(appserv().parent(),visid);
    menus += res;
    res->ref();
    return res;
}


void uiVisPartServer::shareObject( int sceneid, int id )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return;

    mDynamicCastGet(visBase::DataObject*,dobj,visBase::DM().getObj(id))
    if ( !dobj ) return;

    scene->addObject( dobj );
    eventmutex.lock();
    sendEvent( evUpdateTree );
}


void uiVisPartServer::triggerTreeUpdate()
{
    eventmutex.lock();
    sendEvent( evUpdateTree );
}


void uiVisPartServer::findObject( const std::type_info& ti, TypeSet<int>& res )
{
    visBase::DM().getIds( ti, res );
}


visBase::DataObject* uiVisPartServer::getObject( int id ) const
{
    mDynamicCastGet(visBase::DataObject*,dobj,visBase::DM().getObj(id))
    return dobj;
}


void uiVisPartServer::addObject( visBase::DataObject* dobj, int sceneid,
				 bool saveinsessions  )
{
    mDynamicCastGet(visSurvey::Scene*,scene,visBase::DM().getObj(sceneid))
    scene->addObject( dobj );
    //TODO: Handle saveinsessions

    setUpConnections( dobj->id() );
}


void uiVisPartServer::removeObject( visBase::DataObject* dobj, int sceneid )
{
    removeObject( dobj->id(), sceneid );
}


NotifierAccess& uiVisPartServer::removeAllNotifier()
{
    return visBase::DM().removeallnotify;
}


const MultiID* uiVisPartServer::getMultiID( int id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) return so->getMultiID();

    return 0;
}


int uiVisPartServer::getSelObjectId() const
{
    const TypeSet<int>& sel = visBase::DM().selMan().selected();
    return sel.size() ? sel[0] : -1;
}


void uiVisPartServer::setSelObjectId( int id )
{
    visBase::DM().selMan().select( id );
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

    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    if ( so )
	so->getChildren( childids );
}


int uiVisPartServer::getAttributeFormat( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    return so ? so->getAttributeFormat() : -1;
}


CubeSampling uiVisPartServer::getCubeSampling( int id ) const
{
    CubeSampling res;
    mDynamicCastGet( const visSurvey::SurveyObject*, so, getObject(id) );
    if ( so ) res = so->getCubeSampling();
    return res;
}


const AttribSliceSet* uiVisPartServer::getCachedData( int id,
						      bool color ) const
{
    mDynamicCastGet( const visSurvey::SurveyObject*, so, getObject(id) );
    return so ? so->getCacheVolume(color) : 0;
}


bool uiVisPartServer::setCubeData( int id, bool color, AttribSliceSet* sliceset)
{
    if ( !sliceset ) return false;

    mDynamicCastGet( visSurvey::SurveyObject*, so, getObject(id) );
    if ( !so )
    {
	delete sliceset;
	return false;
    }

    uiCursorChanger cursorlock( uiCursor::Wait );
    return so->setDataVolume( color, sliceset );
}


bool uiVisPartServer::canHaveMultipleTextures(int id) const
{
    mDynamicCastGet( const visSurvey::SurveyObject*, so, getObject(id) );
    if ( so ) return so->canHaveMultipleTextures();
    return false;
}


int uiVisPartServer::nrTextures(int id) const
{
    mDynamicCastGet( const visSurvey::SurveyObject*, so, getObject(id) );
    if ( so ) return so->nrTextures();
    return 0;
}


void uiVisPartServer::selectTexture( int id, int textureidx )
{
    mDynamicCastGet( visSurvey::SurveyObject*, so, getObject(id) );
    if ( so ) so->selectTexture(textureidx);
}


void uiVisPartServer::fetchSurfaceData( int id,
				       ObjectSet<BinIDValueSet>& bivs ) const
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so )
	so->fetchData( bivs );
}


void uiVisPartServer::stuffSurfaceData( int id, bool color,
					const ObjectSet<BinIDValueSet>* bp )
{
    mDynamicCastGet( visSurvey::SurveyObject*, so, getObject(id) );
    if ( so )
	so->stuffData( color, bp );
}


void uiVisPartServer::getDataTraceBids( int id, TypeSet<BinID>& bids ) const
{
    mDynamicCastGet( visSurvey::SurveyObject*, so, getObject(id) );
    if ( so )
	so->getDataTraceBids( bids );
}
	

Interval<float> uiVisPartServer::getDataTraceRange( int id ) const
{
    mDynamicCastGet( visSurvey::SurveyObject*, so, getObject(id) );
    return so ? so->getDataTraceRange() : Interval<float>(0,0);
}


void uiVisPartServer::setTraceData( int id, bool color, SeisTrcBuf& data )
{
    mDynamicCastGet( visSurvey::SurveyObject*, so, getObject(id) );
    if ( so )
	so->setTraceData( color, data );
    else
	data.deepErase();
}


Coord3 uiVisPartServer::getMousePos(bool xyt) const
{ return xyt ? xytmousepos : inlcrlmousepos; }


BufferString uiVisPartServer::getMousePosVal() const
{
    return mIsUndefined(mouseposval) ? BufferString("") 
				     : BufferString(mouseposval);
}


BufferString uiVisPartServer::getInteractionMsg( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    return so ? so->getManipulationString() : BufferString("");
}


int uiVisPartServer::getColTabId( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*, so, getObject(id) );
    return so ? so->getColTabID() : -1;
}


void uiVisPartServer::setClipRate( int id, float cr )
{
    mDynamicCastGet(visBase::VisColorTab*,coltab,getObject(getColTabId(id)));
    if ( coltab ) coltab->setClipRate( cr );
}


const TypeSet<float>* uiVisPartServer::getHistogram( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*, so, getObject(id) );
    return so ? so->getHistogram() : 0;
}


int uiVisPartServer::getEventObjId() const { return eventobjid; }


const AttribSelSpec* uiVisPartServer::getSelSpec( int id ) const
{
    mDynamicCastGet( visSurvey::SurveyObject*, so, getObject(id) );
    return so ? so->getSelSpec() : 0;
}


bool uiVisPartServer::deleteAllObjects()
{
    while ( scenes.size() )
	removeScene( scenes[0]->id() );

    scenes.erase();
    for ( int idx=0; idx<menus.size(); idx++ )
	menus[idx]->unRef();

    menus.erase();

    return visBase::DM().reInit();
}


void uiVisPartServer::setViewMode(bool yn)
{
    if ( yn==viewmode ) return;
    viewmode = yn;
    toggleDraggers();
}


bool uiVisPartServer::isViewMode() const { return viewmode; }


void uiVisPartServer::toggleDraggers()
{
    const TypeSet<int>& selected = visBase::DM().selMan().selected();

    for ( int sceneidx=0; sceneidx<scenes.size(); sceneidx++ )
    {
	visSurvey::Scene* scene = scenes[sceneidx];

	for ( int objidx=0; objidx<scene->size(); objidx++ )
	{
	    visBase::DataObject* obj = scene->getObject( objidx );
	    bool isdraggeron = selected.indexOf(obj->id())!=-1 && !viewmode;

	    mDynamicCastGet(visSurvey::SurveyObject*,so,obj)
	    if ( so ) so->showManipulator(isdraggeron);
	}
    }
}
    


void uiVisPartServer::setZScale()
{
    uiZScaleDlg dlg( appserv().parent() );
    dlg.vwallcb = mCB(this,uiVisPartServer,vwAll);
    dlg.homecb = mCB(this,uiVisPartServer,toHome);
    dlg.go();
}


void uiVisPartServer::vwAll( CallBacker* ) { sendEvent( evViewAll ); }
void uiVisPartServer::toHome( CallBacker* ) { sendEvent( evToHomePos ); }


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

	/*
	TypeSet<int> objids;
	getChildIds( sceneid, objids );
	for ( int ido=0; ido<objids.size(); ido++ )
	{
	    visBase::DataObject* dobj = visBase::DM().getObj( objids[ido] );
	    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,dobj)
	    if ( pdd ) pdd->setGeometry( true );
	}
	*/
    }

    return true;
}


bool uiVisPartServer::usePar( const IOPar& par )
{
    BufferString res;
    if ( par.get( workareastr, res ) )
    {
	FileMultiString fms(res);
	CubeSampling cs;
	HorSampling& hs = cs.hrg; StepInterval<float>& zrg = cs.zrg;
	hs.start.inl = atoi(fms[0]); hs.stop.inl = atoi(fms[1]);
	hs.start.crl = atoi(fms[2]); hs.stop.crl = atoi(fms[3]);
	zrg.start = atof(fms[4]); zrg.stop = atof(fms[5]);
	const_cast<SurveyInfo&>(SI()).setRange( cs, true );
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
	addScene( newscene );

	TypeSet<int> children;
	getChildIds( newscene->id(), children );

	for ( int idy=0; idy<children.size(); idy++ )
	{
	    int childid = children[idy];
	    if ( hasconnections.indexOf( childid ) >= 0 ) continue;

	    setUpConnections( childid );
	    hasconnections += childid;

	    turnOn( childid, isOn(childid) );
	}
    }


    float appvel;
    if ( par.get( appvelstr, appvel ) )
	visSurvey::SPM().setZScale( appvel/1000 );

    return true;
}


void uiVisPartServer::calculateAllAttribs()
{
    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	TypeSet<int> children;
	getChildIds( scenes[idx]->id(), children );
	for ( int idy=0; idy<children.size(); idy++ )
	{
	    int childid = children[idy];
	    calculateAttrib( childid, false );
	    calculateColorAttrib( childid, false );
	}
    }
}


void uiVisPartServer::fillPar( IOPar& par ) const
{
    TypeSet<int> storids;

    for ( int idx=0; idx<scenes.size(); idx++ )
	storids += scenes[idx]->id();

    par.set( appvelstr, visSurvey::SPM().getZScale()*1000 );

    const CubeSampling& cs = SI().sampling( true );
    FileMultiString fms;
    fms += cs.hrg.start.inl; fms += cs.hrg.stop.inl; fms += cs.hrg.start.crl;
    fms += cs.hrg.stop.crl; fms += cs.zrg.start; fms += cs.zrg.stop;
    par.set( workareastr, fms );

    visBase::DM().fillPar(par, storids);
}


void uiVisPartServer::turnOn( int id, bool yn )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VisualObject*,vo,dobj)
    if ( vo ) vo->turnOn( yn );

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
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet( const visBase::VisualObject*,vo,dobj)
    return vo ? vo->isOn() : false;
}


bool uiVisPartServer::canDuplicate( int id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so && so->canDuplicate();
}    


int uiVisPartServer::duplicateObject( int id, int sceneid )
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return -1;

    visSurvey::SurveyObject* newso = so->duplicate();
    if ( !newso ) return -1;

    mDynamicCastGet(visBase::DataObject*,doobj,newso)
    addObject( doobj, sceneid, true );
    return doobj->id();
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
    removeConnections( id );

    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return;

    const int idx = scene->getFirstIdx( id );
    if ( idx!=-1 ) 
	scene->removeObject( idx );
}


bool uiVisPartServer::hasAttrib( int id ) const
{
    mDynamicCastGet( visSurvey::SurveyObject*, so, getObject(id) );
    return so && so->getAttributeFormat()!=-1;
}
    

bool uiVisPartServer::selectAttrib( int id )
{
    eventmutex.lock();
    eventobjid = id;
    return sendEvent( evSelectAttrib );
}


void uiVisPartServer::setSelSpec( int id, const AttribSelSpec& myattribspec )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) so->setSelSpec(myattribspec);
}


bool uiVisPartServer::calculateAttrib( int id, bool newselect )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return false;

    const AttribSelSpec* as = so->getSelSpec();
    if ( !as ) return false;
    if ( as->id()==AttribSelSpec::noAttrib )
	return true;

    if ( newselect || ( as->id()==AttribSelSpec::attribNotSel ) )
    {
	if ( !selectAttrib( id ) )
	    return false;
    }

    bool res = false;
    eventmutex.lock();
    eventobjid = id;
    res = sendEvent( evGetNewData );
    if ( res ) so->acceptManipulation();
    return res;
}


bool uiVisPartServer::hasColorAttrib( int id ) const
{
    mDynamicCastGet( visSurvey::SurveyObject*, so, getObject(id) );
    return so && so->getColorSelSpec();
}


const ColorAttribSel* uiVisPartServer::getColorSelSpec( int id ) const
{
    mDynamicCastGet( const visSurvey::SurveyObject*, so, getObject(id) );
    return so ? so->getColorSelSpec() : 0;
}


void uiVisPartServer::setColorSelSpec( int id, const ColorAttribSel& myas )
{
    mDynamicCastGet( visSurvey::SurveyObject*, so, getObject(id) );
    if ( so ) so->setColorSelSpec( myas );
}


void uiVisPartServer::resetColorDataType( int id )
{
    const ColorAttribSel* css = getColorSelSpec(id);
    if ( !css ) return;

    ColorAttribSel myselspec( *css );
    myselspec.datatype = 0;
    setColorSelSpec(id,myselspec);
}


bool uiVisPartServer::calculateColorAttrib( int id, bool newselect )
{
    mDynamicCastGet( visSurvey::SurveyObject*, so, getObject(id));
    if ( !so )
	return false;

    const ColorAttribSel* colas = getColorSelSpec( id );
    if ( !colas ) return false;

    const int attribid = colas->as.id();
    if ( !newselect && attribid < 0 )
	return false;

    bool res = true;
    if ( newselect || ( attribid < 0 ) )
	res = selectColorAttrib( id );
	
    if ( !res ) return res;
    if ( !getColorSelSpec( id )->datatype )
    {
	removeColorData( id );
	return true;
    }

    eventmutex.lock();
    eventobjid = id;
    res = sendEvent( evGetColorData );
    return res;
}


void uiVisPartServer::removeColorData( int id )
{
    mDynamicCastGet( visSurvey::SurveyObject*, so, getObject(id));
    switch ( so->getAttributeFormat() )
    {
	case 0:
	    so->setDataVolume( true, 0 );
	case 1: {
	    SeisTrcBuf dum( false );
	    so->setTraceData( true, dum );
	}
	case 2:
	    so->stuffData( true, 0 );
    }
}


bool uiVisPartServer::selectColorAttrib( int id )
{
    eventmutex.lock();
    bool selected = sendEvent( evSelectColorAttrib );
    return true;
}


bool uiVisPartServer::hasMaterial( int id ) const
{
    mDynamicCastGet( visSurvey::SurveyObject*, so, getObject(id) );
    return so && so->allowMaterialEdit();
}


bool uiVisPartServer::setMaterial( int id )
{
    mDynamicCastGet(visBase::VisualObject*,vo,getObject(id))
    if ( !hasMaterial(id) || !vo ) return false;

    uiMaterialDlg dlg( appserv().parent(), vo->getMaterial(), true,
		       true, false, false, false, true );
    dlg.go();

    return true;
}


bool uiVisPartServer::dumpOI( int id ) const
{
    uiFileDialog filedlg( appserv().parent(), false, GetPersonalDir(), "*.iv",
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
    mDynamicCastGet( visSurvey::SurveyObject*, so, getObject(id) );
    if ( so ) so->resetManipulation();

    eventmutex.lock();
    eventobjid = id;
    sendEvent( evInteraction );

    return so;
}


bool uiVisPartServer::isManipulated( int id ) const
{
    mDynamicCastGet( visSurvey::SurveyObject*, so, getObject(id) );
    return so && so->isManipulated();
}


void uiVisPartServer::acceptManipulation( int id )
{
    mDynamicCastGet( visSurvey::SurveyObject*, so, getObject(id));
    if ( so ) so->acceptManipulation();
}


void uiVisPartServer::setUpConnections( int id )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    NotifierAccess* na = so ? so->getManipulationNotifier() : 0;
    if ( na ) na->notify( mCB(this,uiVisPartServer,interactionCB) );

    uiVisMenu* menu = getMenu(id,true);
    menu->createnotifier.notify( mCB(this,uiVisPartServer,createMenuCB) );
    menu->handlenotifier.notify( mCB(this,uiVisPartServer,handleMenuCB) );

    mDynamicCastGet(visBase::VisualObject*,vo,getObject(id))
    if ( vo && vo->rightClicked() )
	vo->rightClicked()->notify(mCB(this,uiVisPartServer,rightClickCB));
}


void uiVisPartServer::removeConnections( int id )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    NotifierAccess* na = so ? so->getManipulationNotifier() : 0;

    if ( na ) na->remove( mCB(this,uiVisPartServer,interactionCB) );

    uiVisMenu* menu = getMenu(id,false);
    int mnufactidx = menus.indexOf(menu);
    menus.remove(mnufactidx);
    menu->createnotifier.remove( mCB(this,uiVisPartServer,createMenuCB) );
    menu->handlenotifier.remove( mCB(this,uiVisPartServer,handleMenuCB) );

    mDynamicCastGet(visBase::VisualObject*,vo,getObject(id))
    if ( vo && vo->rightClicked() )
	vo->rightClicked()->remove(mCB(this,uiVisPartServer,rightClickCB));
    menu->unRef();
}


void uiVisPartServer::rightClickCB( CallBacker* cb )
{
    mDynamicCastGet( visBase::DataObject*,dataobj,cb );
    const int id = dataobj ? dataobj->id() : getSelObjectId();
    if ( id==-1 )
	return;

    showMenu( id, 1, dataobj ? dataobj->rightClickedPath() : 0 );
}


void uiVisPartServer::selectObjCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sel,cb);
    visBase::DataObject* dobj = visBase::DM().getObj( sel );
    mDynamicCastGet(visSurvey::SurveyObject*,so,dobj);
    if ( !viewmode && so )
	so->showManipulator(true);

    eventmutex.lock();
    eventobjid = sel;
    sendEvent( evSelection );
}


void uiVisPartServer::deselectObjCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,oldsel,cb);
    visBase::DataObject* dobj = visBase::DM().getObj( oldsel );
    mDynamicCastGet(visSurvey::SurveyObject*,so,dobj)
    if ( so )
    {    
	if ( so->getAttributeFormat() == 3 )
	{
	    if ( !so->isManipulated() ) return;
	    eventmutex.lock();
	    eventobjid = oldsel;
	    sendEvent( evGetNewData );
	    return;
	}

	if ( so->isManipulated() )
	{
	    calculateAttrib( oldsel, false );
	    calculateColorAttrib( oldsel, false );
	}

	if ( !viewmode )
	    so->showManipulator(false);
    }

    eventmutex.lock();
    eventobjid = oldsel;
    sendEvent( evDeSelection );
}


void uiVisPartServer::interactionCB( CallBacker* cb )
{
    mDynamicCastGet(visBase::DataObject*,dataobj,cb)
    if ( dataobj )
    {
	eventmutex.lock();
	eventobjid = dataobj->id();
	sendEvent( evInteraction );
    }
}


void uiVisPartServer::mouseMoveCB( CallBacker* cb )
{
    mDynamicCastGet(visSurvey::Scene*,scene,cb)
    if ( !scene ) return;

    eventmutex.lock();
    xytmousepos = scene->getMousePos(true);
    inlcrlmousepos = scene->getMousePos(false);
    mouseposval = scene->getMousePosValue();
    sendEvent( evMouseMove );
}


void uiVisPartServer::createMenuCB(CallBacker* cb)
{
    mDynamicCastGet( uiVisMenu*, menu, cb );
    mDynamicCastGet( visSurvey::SurveyObject*, so, getObject(menu->id()));
    mDynamicCastGet( visBase::VisualObject*, vo, getObject(menu->id()));
    if ( !so ) return;

    selcolorattrmnusel = so->getColorSelSpec()
	?  menu->addItem( new uiMenuItem("Select color attribute ..."), 9000 ) 
	: -1;

    resetmanipmnusel = so->isManipulated() && so->canResetManipulation()
	? menu->addItem( new uiMenuItem("Reset Manipulation"), 8000 )
	: -1;

    changecolormnusel = so->hasColor()
	? menu->addItem( new uiMenuItem("Color..."), 7000 )
	: -1;

    changematerialmnusel = so->allowMaterialEdit()
	? menu->addItem( new uiMenuItem("Properties ..."), 6000 )
	: -1;

    if ( so->nrResolutions()>1 )
    {
	uiPopupMenu* resmnu = new uiPopupMenu( menu->getParent(),"Resolution");
	for ( int idx=0; idx<so->nrResolutions(); idx++ )
	{
	    int mid = menu->getFreeID();
	uiMenuItem* itm = new uiMenuItem(so->getResolutionName(idx));
	resmnu->insertItem(itm, mid);
	itm->setChecked(idx==so->getResolution());
	if ( !idx ) firstresmnusel = mid;
	}

	menu->addSubMenu( resmnu, 5000 );
    }

    mDynamicCastGet(visSurvey::SurfaceInterpreterDisplay*,sid,so);
    if ( sid && !trackdlg )
    {
	uiVisMenu* menu = getMenu( sid->id(), false );
	trackmanmnuid = menu->addItem( new uiMenuItem("Show manager") );
    }
    else
	trackmanmnuid = -1;
}


void uiVisPartServer::handleMenuCB(CallBacker* cb)
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    if ( mnuid==-1 ) return;

    mDynamicCastGet( uiVisMenu*, menu, caller );
    const int id = menu->id();
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    if ( !so ) return;
    
    if ( mnuid==selcolorattrmnusel )
	calculateColorAttrib( id, true );
    else if ( mnuid==resetmanipmnusel )
	resetManipulation(id);
    else if ( mnuid==changecolormnusel )
    {
	Color col = so->getColor();
	if ( select( col, appserv().parent(), "Color selection", false ) )
	    so->setColor( col );
    }
    else if ( mnuid==changematerialmnusel )
	setMaterial(id);
    else if ( firstresmnusel!=-1 && mnuid>=firstresmnusel &&
	    mnuid-firstresmnusel<so->nrResolutions() )
    {
	uiCursorChanger cursorlock( uiCursor::Wait );
	so->setResolution(mnuid-firstresmnusel);
    }
    else if ( mnuid==trackmanmnuid )
    {
	mDynamicCastGet(visSurvey::SurfaceInterpreterDisplay*,sid,so)
	if ( sid && sid->trackMan() )
	    showTrackingManager( sid->id(), *sid->trackMan() );
    }
}


int uiVisPartServer::addInterpreter( int sceneid, Tracking::TrackManager& tm )
{
    visSurvey::SurfaceInterpreterDisplay* sid =
				visSurvey::SurfaceInterpreterDisplay::create();
    sid->setTrackMan( tm );
    sid->turnOn( false );
    addObject( sid, sceneid, true );
    return sid->id();
}


void uiVisPartServer::showTrackingManager( int id, Tracking::TrackManager& tm )
{
    delete trackdlg;
    mDynamicCastGet(visSurvey::SurfaceInterpreterDisplay*,sid,getObject(id))
    trackdlg = new uiTrackingMan( appserv().parent(), *sid, tm );
    trackdlg->windowClosed.notify( mCB(this,uiVisPartServer,trackmanDlgClosed));
    trackdlg->go();
}


void uiVisPartServer::trackmanDlgClosed( CallBacker* )
{
    delete trackdlg; trackdlg = 0;
}
