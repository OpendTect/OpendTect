/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: vissurvscene.cc,v 1.73 2005-10-26 21:59:03 cvskris Exp $
________________________________________________________________________

-*/

#include "vissurvscene.h"

#include "iopar.h"
#include "survinfo.h"
#include "cubesampling.h"
#include "visannot.h"
#include "visdataman.h"
#include "visevent.h"
#include "vistransform.h"
#include "vistransmgr.h"
#include "vissurvobj.h"
#include "zaxistransform.h"


mCreateFactoryEntry( visSurvey::Scene );


namespace visSurvey {

const char* Scene::annottxtstr = "Show text";
const char* Scene::annotscalestr = "Show scale";
const char* Scene::annotcubestr = "Show cube";

Scene::Scene()
    : inlcrl2disptransform(0)
    , utm2disptransform(0)
    , zscaletransform(0)
    , annot(0)
    , mouseposchange(this)
    , mouseposval(0)
    , mouseposstr("")
    , zscalechange(this)
    , curzscale(STM().defZScale())
    , datatransform( 0 )
{
    events.eventhappened.notify( mCB(this,Scene,mouseMoveCB) );
    setAmbientLight( 1 );
    init();
}


void Scene::init()
{
    annot = visBase::Annotation::create();
    annot->setText( 0, "In-line" );
    annot->setText( 1, "Cross-line" );
    annot->setText( 2, SI().zIsTime() ? "TWT" : "Depth" );

    const CubeSampling& cs = SI().sampling(true);
    createTransforms( cs.hrg );
    float zsc = STM().defZScale();
    SI().pars().get( STM().zScaleStr(), zsc );
    setZScale( zsc );
    
    setAnnotationCube( cs );
    addInlCrlTObject( annot );
}


Scene::~Scene()
{
    events.eventhappened.remove( mCB(this,Scene,mouseMoveCB) );

    int objidx = getFirstIdx( inlcrl2disptransform );
    if ( objidx >= 0 ) removeObject( objidx );

    objidx = getFirstIdx( zscaletransform );
    if ( objidx >= 0 ) removeObject( objidx );

    objidx = getFirstIdx( annot );
    if ( objidx >= 0 ) removeObject( objidx );

    if ( utm2disptransform ) utm2disptransform->unRef();
    if ( datatransform ) datatransform->unRef();
}


void Scene::createTransforms( const HorSampling& hs )
{
    if ( !zscaletransform )
    {
	zscaletransform = STM().createZScaleTransform();
	visBase::DataObjectGroup::addObject(
		const_cast<visBase::Transformation*>(zscaletransform) );
    }

    if ( !inlcrl2disptransform )
    {
	inlcrl2disptransform = STM().createIC2DisplayTransform( hs );
	visBase::DataObjectGroup::addObject(
		const_cast<visBase::Transformation*>(inlcrl2disptransform) );
    }

    if ( !utm2disptransform )
    {
	utm2disptransform = STM().createUTM2DisplayTransform( hs );
	utm2disptransform->ref();
    }
}


void Scene::setAnnotationCube( const CubeSampling& cs )
{
    if ( !annot ) return;

    BinID c0( cs.hrg.start.inl, cs.hrg.start.crl ); 
    BinID c1( cs.hrg.stop.inl, cs.hrg.start.crl ); 
    BinID c2( cs.hrg.stop.inl, cs.hrg.stop.crl ); 
    BinID c3( cs.hrg.start.inl, cs.hrg.stop.crl );

    annot->setCorner( 0, c0.inl, c0.crl, cs.zrg.start );
    annot->setCorner( 1, c1.inl, c1.crl, cs.zrg.start );
    annot->setCorner( 2, c2.inl, c2.crl, cs.zrg.start );
    annot->setCorner( 3, c3.inl, c3.crl, cs.zrg.start );
    annot->setCorner( 4, c0.inl, c0.crl, cs.zrg.stop );
    annot->setCorner( 5, c1.inl, c1.crl, cs.zrg.stop );
    annot->setCorner( 6, c2.inl, c2.crl, cs.zrg.stop );
    annot->setCorner( 7, c3.inl, c3.crl, cs.zrg.stop );
}


void Scene::addUTMObject( visBase::VisualObject* obj )
{
    obj->setDisplayTransformation( utm2disptransform );
    const int insertpos = getFirstIdx( inlcrl2disptransform );
    insertObject( insertpos, obj );
}


void Scene::addInlCrlTObject( visBase::DataObject* obj )
{
    visBase::Scene::addObject( obj );
}


void Scene::addObject( visBase::DataObject* obj )
{
    mDynamicCastGet(SurveyObject*,so,obj)
    mDynamicCastGet(visBase::VisualObject*,vo,obj)

    if ( so && so->getMovementNotification() )
	so->getMovementNotification()->notify( mCB(this,Scene,objectMoved) );

    if ( so )
    {
	if ( so->getMovementNotification() )
	    so->getMovementNotification()->notify( mCB(this,Scene,objectMoved));

	so->setScene( this );
    }

    if ( so && so->isInlCrl() )
	addInlCrlTObject( obj );
    else if ( vo )
	addUTMObject( vo );
}


void Scene::removeObject( int idx )
{
    DataObject* obj = getObject( idx );
    mDynamicCastGet(SurveyObject*,so,obj)
    if ( so && so->getMovementNotification() )
	so->getMovementNotification()->remove( mCB(this,Scene,objectMoved) );

    visBase::DataObjectGroup::removeObject( idx );
}


void Scene::setZScale( float zscale )
{
    if ( !zscaletransform ) return;
    STM().setZScale( zscaletransform, zscale );
    curzscale = zscale;
}


void Scene::showAnnotText( bool yn )	{ annot->showText( yn ); }
bool Scene::isAnnotTextShown() const	{ return annot->isTextShown(); }
void Scene::showAnnotScale( bool yn )	{ annot->showScale( yn ); }
bool Scene::isAnnotScaleShown() const	{ return annot->isScaleShown(); }
void Scene::showAnnot( bool yn )	{ annot->turnOn( yn ); }
bool Scene::isAnnotShown() const	{ return annot->isOn(); }


Coord3 Scene::getMousePos( bool xyt ) const
{
   if ( xyt ) return xytmousepos;
   
   Coord3 res = xytmousepos;
   BinID binid = SI().transform( Coord(res.x,res.y) );
   res.x = binid.inl;
   res.y = binid.crl;
   return res;
}


void Scene::objectMoved( CallBacker* cb )
{
    ObjectSet<const SurveyObject> activeobjects;
    int movedid = -1;
    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(SurveyObject*,so,getObject(idx))
	if ( !so ) continue;
	if ( !so->getMovementNotification() ) continue;

	mDynamicCastGet(visBase::VisualObject*,vo,getObject(idx))
	if ( !vo ) continue;
	if ( !vo->isOn() ) continue;

	if ( cb==vo ) movedid = vo->id();

	activeobjects += so;
    }

    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(SurveyObject*,so,getObject(idx));
	
	if ( so ) so->otherObjectsMoved( activeobjects, movedid );
    }
}


void Scene::mouseMoveCB( CallBacker* cb )
{
    STM().setCurrentScene( this );

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
    if ( eventinfo.type != visBase::MouseMovement ) return;

    const int sz = eventinfo.pickedobjids.size();
    bool validpicksurface = false;
    const Coord3 displayspacepos = 
	    zscaletransform->transformBack( eventinfo.pickedpos );
    xytmousepos = utm2disptransform->transformBack( displayspacepos );

    mouseposval = mUndefValue;
    mouseposstr = "";
    for ( int idx=0; idx<sz; idx++ )
    {
	const DataObject* pickedobj =
			visBase::DM().getObject(eventinfo.pickedobjids[idx]);
	mDynamicCastGet(const SurveyObject*,so,pickedobj)
	if ( so )
	{
	    if ( !validpicksurface )
		validpicksurface = true;

	    if ( mIsUndefined(mouseposval) )
	    {
		float newmouseposval;
		BufferString newstr;
		so->getMousePosInfo( eventinfo, xytmousepos, newmouseposval,
				     newstr );
		if ( newstr != "" )
		    mouseposstr = newstr;
		if ( !mIsUndefined(newmouseposval) )
		    mouseposval = newmouseposval;
	    }
	    else if ( validpicksurface )
		break;
	}
    }

    if ( validpicksurface )
	mouseposchange.trigger();
}


void Scene::setDataTransform( ZAxisTransform* zat )
{
    if ( datatransform ) datatransform->unRef();
    datatransform = zat;
    if ( datatransform ) datatransform->ref();

    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(SurveyObject*,so,getObject(idx))
	if ( !so ) continue;

	so->setDataTransform( zat );
    }
}


ZAxisTransform* Scene::getDataTransform()
{ return datatransform; }


void Scene::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::DataObject::fillPar( par, saveids );

    int kid = 0;
    while ( getObject(kid++) != zscaletransform )
	;

    int nrkids = 0;    
    for ( ; kid<size(); kid++ )
    {
	if ( getObject(kid)==annot || 
	     getObject(kid)==inlcrl2disptransform ) continue;

	const int objid = getObject(kid)->id();
	if ( saveids.indexOf(objid) == -1 )
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


void Scene::removeAll()
{
    visBase::DataObjectGroup::removeAll();
    zscaletransform = 0; inlcrl2disptransform = 0; annot = 0;
}


int Scene::usePar( const IOPar& par )
{
    removeAll();
    init();

    int res = visBase::DataObjectGroup::usePar( par );
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

} // namespace visSurvey
