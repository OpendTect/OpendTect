/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vissurvscene.cc,v 1.52 2003-11-24 10:51:53 kristofer Exp $";

#include "vissurvscene.h"

#include "errh.h"
#include "iopar.h"
#include "linsolv.h"
#include "survinfo.h"
#include "visannot.h"
#include "visdataman.h"
#include "visevent.h"
#include "vislight.h"
#include "visplanedatadisplay.h"
#include "visrandomtrackdisplay.h"
#include "vissurvinterpret.h"
#include "vissurvpickset.h"
#include "vissurvsurf.h"
#include "vistransform.h"
#include "visvolumedisplay.h"

#include <limits.h>

mCreateFactoryEntry( visSurvey::Scene );

const char* visSurvey::Scene::annottxtstr = "Show text";
const char* visSurvey::Scene::annotscalestr = "Show scale";
const char* visSurvey::Scene::annotcubestr = "Show cube";

const char* visSurvey::Scene::displobjprefixstr = "Displ Object ";
const char* visSurvey::Scene::nodisplobjstr = "No Displ Objects";
const char* visSurvey::Scene::xyzobjprefixstr = "XYZ Object ";
const char* visSurvey::Scene::noxyzobjstr = "No XYZ Objects";
const char* visSurvey::Scene::xytobjprefixstr = "XYT Object ";
const char* visSurvey::Scene::noxytobjstr = "No XYT Objects";
const char* visSurvey::Scene::inlcrltobjprefixstr = "InlCrl Object ";
const char* visSurvey::Scene::noinlcrltobjstr = "No InlCrl Objects";

visSurvey::Scene::Scene()
    : inlcrl2displtransform( SPM().getInlCrl2DisplayTransform() )
    , zscaletransform( SPM().getZScaleTransform() )
    , annot( 0 )
    , eventcatcher( 0 )
    , mouseposchange( this )
    , mouseposval(0)
{
    setAmbientLight( 1 );
    setup();
}


visSurvey::Scene::~Scene()
{
    eventcatcher->eventhappened.remove( mCB( this, Scene, mouseMoveCB ));
}


void visSurvey::Scene::updateRange()
{ setCube(); }


void visSurvey::Scene::setCube()
{
    if ( !annot ) return;
    BinIDRange hrg = SI().range();
    StepInterval<double> vrg = SI().zRange();

    BinID c0( hrg.start.inl, hrg.start.crl ); 
    BinID c1( hrg.stop.inl, hrg.start.crl ); 
    BinID c2( hrg.stop.inl, hrg.stop.crl ); 
    BinID c3( hrg.start.inl, hrg.stop.crl );

    annot->setCorner( 0, c0.inl, c0.crl, vrg.start );
    annot->setCorner( 1, c1.inl, c1.crl, vrg.start );
    annot->setCorner( 2, c2.inl, c2.crl, vrg.start );
    annot->setCorner( 3, c3.inl, c3.crl, vrg.start );
    annot->setCorner( 4, c0.inl, c0.crl, vrg.stop );
    annot->setCorner( 5, c1.inl, c1.crl, vrg.stop );
    annot->setCorner( 6, c2.inl, c2.crl, vrg.stop );
    annot->setCorner( 7, c3.inl, c3.crl, vrg.stop );
}


void visSurvey::Scene::addUTMObject( visBase::VisualObject* obj )
{
    obj->setTransformation( SPM().getUTM2DisplayTransform() );
    int insertpos = getFirstIdx( inlcrl2displtransform );
    insertObject( insertpos, obj );
}


void visSurvey::Scene::addInlCrlTObject( visBase::SceneObject* obj )
{
    visBase::SceneObjectGroup::addObject( obj );
}


void  visSurvey::Scene::addObject( visBase::SceneObject* sobj )
{
    mDynamicCastGet( SurveyObject*, survobj, sobj );
    if ( survobj && survobj->getMovementNotification() )
    {
	survobj->getMovementNotification()->notify(
		mCB( this,visSurvey::Scene,filterPicks ));
    }

    if ( survobj && survobj->isInlCrl() )
    {
	addInlCrlTObject( sobj );
	return;
    }

    mDynamicCastGet( visBase::VisualObject*, visobj, sobj );
    if ( visobj )
    {
	addUTMObject(visobj);
	return;
    }
}


void visSurvey::Scene::removeObject( int idx )
{
    SceneObject* obj = getObject( idx );
    mDynamicCastGet( SurveyObject*, survobj, obj );
    if ( survobj && survobj->getMovementNotification() )
    {
	survobj->getMovementNotification()->remove(
		mCB( this,visSurvey::Scene,filterPicks ));
    }

    SceneObjectGroup::removeObject( idx );
}


void visSurvey::Scene::showAnnotText( bool yn )
{
    annot->showText( yn );
}


bool  visSurvey::Scene::isAnnotTextShown() const
{
    return annot->isTextShown();
}


void visSurvey::Scene::showAnnotScale( bool yn )
{
    annot->showScale( yn );
}


bool visSurvey::Scene::isAnnotScaleShown() const
{
    return annot->isScaleShown();
}


void visSurvey::Scene::showAnnot( bool yn )
{
    annot->turnOn( yn );
}


bool visSurvey::Scene::isAnnotShown() const
{
    return annot->isOn();
}


Coord3 visSurvey::Scene::getMousePos( bool xyt ) const
{
   if ( xyt ) return xytmousepos;
   
   Coord3 res = xytmousepos;
   BinID binid = SI().transform( Coord( res.x, res.y ));

   res.x = binid.inl;
   res.y = binid.crl;
   return res;
}


void visSurvey::Scene::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::SceneObject::fillPar( par, saveids );

    int kid = 0;
    while ( getObject(kid++)!= zscaletransform )
	;

    int nrkids = 0;    
    for ( ; kid<size(); kid++ )
    {
	if ( getObject(kid)==annot ) continue;
	if ( getObject(kid)==inlcrl2displtransform )
	{
	    par.set(xytobjprefixstr,nrkids);
	    continue;
	}

	const int objid = getObject(kid)->id();

	if ( saveids.indexOf(objid)==-1 )
	    saveids += objid;

	BufferString key = kidprefix; key += nrkids;
	par.set( key, objid );

	nrkids++;
    }

    par.set( nokidsstr, nrkids );
    
    bool txtshown = isAnnotTextShown();
    par.setYN( annottxtstr, txtshown );
    bool scaleshown = isAnnotScaleShown();
    par.setYN( annotscalestr, scaleshown );
    bool cubeshown = isAnnotShown();
    par.setYN( annotcubestr, cubeshown );
}


int visSurvey::Scene::usePar( const IOPar& par )
{
    eventcatcher->eventhappened.remove( mCB( this, Scene, mouseMoveCB ));
    removeAll();
    setup();

/* This is only to comply to old session format that dissappeared in
   dTect version 1.5
*/
    int res = useOldPar( par );
    if ( res!=1 ) return res;
// End of old session format

    res = visBase::SceneObjectGroup::usePar( par );
    if ( res!=1 ) return res;

    bool txtshown = true;
    par.getYN( annottxtstr, txtshown );
    showAnnotText( txtshown );

    bool scaleshown = true;
    par.getYN( annotscalestr, scaleshown );
    showAnnotScale( scaleshown );

    bool cubeshown = true;
    par.getYN( annotcubestr, cubeshown );
    showAnnot( cubeshown );

    return 1;
}


int visSurvey::Scene::useOldPar( const IOPar& par )
{
    int nrdisplobj = 0;
    if ( !par.get( nodisplobjstr, nrdisplobj ))
	// OK, this seems to be a new par-file
	return 1;

    TypeSet<int> displobjids( nrdisplobj, -1 );
    for ( int idx=0; idx<displobjids.size(); idx++ )
    {
	BufferString key = displobjprefixstr;
	key += idx;
	if ( !par.get( key, displobjids[idx] )) return -1;
	if ( !visBase::DM().getObj( displobjids[idx] ) ) return 0;
    }

    int nrxyzobj;
    if ( !par.get( noxyzobjstr, nrxyzobj ))
	// old par is erronious
	return -1;

    TypeSet<int> xyzobjids( nrxyzobj, -1 );
    for ( int idx=0; idx<xyzobjids.size(); idx++ )
    {
	BufferString key = xyzobjprefixstr;
	key += idx;
	if ( !par.get( key, xyzobjids[idx] )) return -1;
	if ( !visBase::DM().getObj( xyzobjids[idx] ) ) return 0;
    }

    int nrxytobj;
    if ( !par.get( noxytobjstr, nrxytobj ))
	// old par is erronious
        return -1;

    TypeSet<int> xytobjids( nrxytobj, -1 );
    for ( int idx=0; idx<xytobjids.size(); idx++ )
    {
	BufferString key = xytobjprefixstr;
	key += idx;
	if ( !par.get( key, xytobjids[idx] )) return -1;
	if ( !visBase::DM().getObj( xytobjids[idx] ) ) return 0;
    }

    int noinlcrltobj;
    if ( !par.get( noinlcrltobjstr, noinlcrltobj ))
	// old par is erronious
        return -1;

    TypeSet<int> inlcrlobjids( noinlcrltobj, -1 );
    for ( int idx=0; idx<inlcrlobjids.size(); idx++ )
    {
	BufferString key = inlcrltobjprefixstr;
	key += idx;

	if ( !par.get( key, inlcrlobjids[idx] )) return -1;
	if ( !visBase::DM().getObj( inlcrlobjids[idx] ) ) return 0;
    }

    for ( int idx=0; idx<displobjids.size(); idx++ )
    {
	mDynamicCastGet( visBase::SceneObject*, so,
	    visBase::DM().getObj( displobjids[idx] ));
	if ( so ) addObject( so );
    }

    for ( int idx=0; idx<xyzobjids.size(); idx++ )
    {
	mDynamicCastGet( visBase::SceneObject*, so,
	    visBase::DM().getObj( xyzobjids[idx] ));
	if ( so ) addObject( so );
    }

    for ( int idx=0; idx<xytobjids.size(); idx++ )
    {
	mDynamicCastGet( visBase::SceneObject*, so,
	    visBase::DM().getObj( xytobjids[idx] ));
	if ( so ) addObject( so );
    }
    for ( int idx=0; idx<inlcrlobjids.size(); idx++ )
    {
	mDynamicCastGet( visBase::SceneObject*, so,
	    visBase::DM().getObj( inlcrlobjids[idx] ));
	if ( so ) addObject( so );
    }

    return 1;
}


void visSurvey::Scene::setup()
{
    setAmbientLight( 1 );

    eventcatcher = visBase::EventCatcher::create();
    visBase::SceneObjectGroup::addObject( eventcatcher );
    eventcatcher->setEventType( visBase::MouseMovement );
    eventcatcher->eventhappened.notify( mCB( this, Scene, mouseMoveCB ));

    visBase::SceneObjectGroup::addObject(
	    	const_cast<visBase::Transformation*>(zscaletransform));
    visBase::SceneObjectGroup::addObject(
	    const_cast<visBase::Transformation*>(inlcrl2displtransform) );

    annot = visBase::Annotation::create();
    annot->setText( 0, "In-line" );
    annot->setText( 1, "Cross-line" );
    annot->setText( 2, SI().zIsTime() ? "TWT" : "Depth" );
    addInlCrlTObject( annot );
    setCube();
}


void visSurvey::Scene::filterPicks(CallBacker* cb)
{
    ObjectSet<SurveyObject> activeobjects;
    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet( SurveyObject*, survobj, getObject( idx ) );
	if ( !survobj ) continue;
	if ( !survobj->getMovementNotification() ) continue;

	mDynamicCastGet( visBase::VisualObject*, visobj, getObject( idx ) );
	if ( !visobj ) continue;
	if ( !visobj->isOn() ) continue;

	activeobjects += survobj;
    }

    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet( PickSetDisplay*, pickset, getObject( idx ) );
	if ( pickset ) pickset->filterPicks( activeobjects, 10 );
    }
}


void visSurvey::Scene::mouseMoveCB(CallBacker* cb )
{
    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb );

    if ( eventinfo.type != visBase::MouseMovement ) return;

    const int sz = eventinfo.pickedobjids.size();
    bool validpicksurface = false;
    const visSurvey::PlaneDataDisplay* pdd = 0;
    const visSurvey::SurfaceDisplay* sd =0;
    const visSurvey::VolumeDisplay* vd = 0;
    const visSurvey::RandomTrackDisplay* rtd = 0;

    for ( int idx=0; idx<sz; idx++ )
    {
	const DataObject* pickedobj =
			    visBase::DM().getObj(eventinfo.pickedobjids[idx]);

	if ( typeid(*pickedobj) == typeid(visSurvey::PlaneDataDisplay) )
	{
	    validpicksurface = true;
	    pdd = (const visSurvey::PlaneDataDisplay*) pickedobj;
	    break;
	}

	if ( typeid(*pickedobj) == typeid(visSurvey::SurfaceDisplay) )
	{
	    validpicksurface = true;
	    sd = (const visSurvey::SurfaceDisplay*) pickedobj;
	    break;
	}

	if ( typeid(*pickedobj) == typeid(visSurvey::VolumeDisplay) )
	{
	    validpicksurface = true;
	    vd = (const visSurvey::VolumeDisplay*) pickedobj;
	    break;
	}

	if ( typeid(*pickedobj) == typeid(visSurvey::RandomTrackDisplay) )
	{
	    validpicksurface = true;
	    rtd = (const visSurvey::RandomTrackDisplay*) pickedobj;
	    break;
	}
    }

    if ( !validpicksurface ) return;

    Coord3 displayspacepos =
	SPM().getZScaleTransform()->transformBack(eventinfo.pickedpos);


    xytmousepos =
	SPM().getUTM2DisplayTransform()->transformBack(displayspacepos);

    Coord3 inlcrl = xytmousepos;
    BinID binid = SI().transform( Coord( xytmousepos.x, xytmousepos.y ));
    inlcrl.x = binid.inl;
    inlcrl.y = binid.crl;

    if ( pdd )
	mouseposval = pdd->getValue( inlcrl );
    else if ( sd )
	mouseposval = sd->getValue( xytmousepos );
    else if ( vd )
	mouseposval = vd->getValue( inlcrl );
    else if ( rtd )
	mouseposval = rtd->getValue( inlcrl );
    mouseposchange.trigger();
}
