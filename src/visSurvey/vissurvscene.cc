/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vissurvscene.cc,v 1.56 2004-06-23 10:39:52 nanne Exp $";

#include "vissurvscene.h"

#include "errh.h"
#include "iopar.h"
#include "linsolv.h"
#include "survinfo.h"
#include "visannot.h"
#include "visdataman.h"
#include "visevent.h"
#include "vislight.h"
#include "vissurvpickset.h"
#include "vistransform.h"

#include <limits.h>

mCreateFactoryEntry( visSurvey::Scene );


namespace visSurvey {

const char* Scene::annottxtstr = "Show text";
const char* Scene::annotscalestr = "Show scale";
const char* Scene::annotcubestr = "Show cube";

const char* Scene::displobjprefixstr = "Displ Object ";
const char* Scene::nodisplobjstr = "No Displ Objects";
const char* Scene::xyzobjprefixstr = "XYZ Object ";
const char* Scene::noxyzobjstr = "No XYZ Objects";
const char* Scene::xytobjprefixstr = "XYT Object ";
const char* Scene::noxytobjstr = "No XYT Objects";
const char* Scene::inlcrltobjprefixstr = "InlCrl Object ";
const char* Scene::noinlcrltobjstr = "No InlCrl Objects";

Scene::Scene()
    : inlcrl2displtransform( SPM().getInlCrl2DisplayTransform() )
    , zscaletransform( SPM().getZScaleTransform() )
    , annot(0)
    , eventcatcher(0)
    , mouseposchange(this)
    , mouseposval(0)
{
    setAmbientLight( 1 );
    setup();
}


Scene::~Scene()
{
    eventcatcher->eventhappened.remove( mCB(this,Scene,mouseMoveCB) );
}


void Scene::updateRange()
{ setCube(); }


void Scene::setCube()
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


void Scene::addUTMObject( visBase::VisualObject* obj )
{
    obj->setTransformation( SPM().getUTM2DisplayTransform() );
    int insertpos = getFirstIdx( inlcrl2displtransform );
    insertObject( insertpos, obj );
}


void Scene::addInlCrlTObject( visBase::DataObject* obj )
{
    visBase::DataObjectGroup::addObject( obj );
}


void Scene::addObject( visBase::DataObject* obj )
{
    mDynamicCastGet(SurveyObject*,so,obj)
    if ( so && so->getMovementNotification() )
    {
	so->getMovementNotification()->notify(
		mCB(this,Scene,filterPicks) );
    }

    if ( so && so->isInlCrl() )
    {
	addInlCrlTObject( obj );
	return;
    }

    mDynamicCastGet(visBase::VisualObject*,vo,obj)
    if ( vo )
    {
	addUTMObject( vo );
	return;
    }
}


void Scene::removeObject( int idx )
{
    DataObject* obj = getObject( idx );
    mDynamicCastGet(SurveyObject*,so,obj)
    if ( so && so->getMovementNotification() )
    {
	so->getMovementNotification()->remove(
		mCB(this,Scene,filterPicks) );
    }

    DataObjectGroup::removeObject( idx );
}


void Scene::showAnnotText( bool yn )
{
    annot->showText( yn );
}


bool Scene::isAnnotTextShown() const
{
    return annot->isTextShown();
}


void Scene::showAnnotScale( bool yn )
{
    annot->showScale( yn );
}


bool Scene::isAnnotScaleShown() const
{
    return annot->isScaleShown();
}


void Scene::showAnnot( bool yn )
{
    annot->turnOn( yn );
}


bool Scene::isAnnotShown() const
{
    return annot->isOn();
}


Coord3 Scene::getMousePos( bool xyt ) const
{
   if ( xyt ) return xytmousepos;
   
   Coord3 res = xytmousepos;
   BinID binid = SI().transform( Coord( res.x, res.y ));

   res.x = binid.inl;
   res.y = binid.crl;
   return res;
}


void Scene::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::DataObject::fillPar( par, saveids );

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


int Scene::usePar( const IOPar& par )
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

    res = visBase::DataObjectGroup::usePar( par );
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


int Scene::useOldPar( const IOPar& par )
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
	mDynamicCastGet( visBase::DataObject*, so,
	    visBase::DM().getObj( displobjids[idx] ));
	if ( so ) addObject( so );
    }

    for ( int idx=0; idx<xyzobjids.size(); idx++ )
    {
	mDynamicCastGet( visBase::DataObject*, so,
	    visBase::DM().getObj( xyzobjids[idx] ));
	if ( so ) addObject( so );
    }

    for ( int idx=0; idx<xytobjids.size(); idx++ )
    {
	mDynamicCastGet( visBase::DataObject*, so,
	    visBase::DM().getObj( xytobjids[idx] ));
	if ( so ) addObject( so );
    }
    for ( int idx=0; idx<inlcrlobjids.size(); idx++ )
    {
	mDynamicCastGet( visBase::DataObject*, so,
	    visBase::DM().getObj( inlcrlobjids[idx] ));
	if ( so ) addObject( so );
    }

    return 1;
}


void Scene::setup()
{
    setAmbientLight( 1 );

    eventcatcher = visBase::EventCatcher::create();
    visBase::DataObjectGroup::addObject( eventcatcher );
    eventcatcher->setEventType( visBase::MouseMovement );
    eventcatcher->eventhappened.notify( mCB( this, Scene, mouseMoveCB ));

    visBase::DataObjectGroup::addObject(
	    	const_cast<visBase::Transformation*>(zscaletransform));
    visBase::DataObjectGroup::addObject(
	    const_cast<visBase::Transformation*>(inlcrl2displtransform) );

    annot = visBase::Annotation::create();
    annot->setText( 0, "In-line" );
    annot->setText( 1, "Cross-line" );
    annot->setText( 2, SI().zIsTime() ? "TWT" : "Depth" );
    addInlCrlTObject( annot );
    setCube();
}


void Scene::filterPicks( CallBacker* cb )
{
    ObjectSet<SurveyObject> activeobjects;
    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(SurveyObject*,so,getObject(idx))
	if ( !so ) continue;
	if ( !so->getMovementNotification() ) continue;

	mDynamicCastGet(visBase::VisualObject*,vo,getObject(idx))
	if ( !vo ) continue;
	if ( !vo->isOn() ) continue;

	activeobjects += so;
    }

    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(PickSetDisplay*,pickset,getObject(idx))
	if ( pickset ) pickset->filterPicks( activeobjects );
    }
}


void Scene::mouseMoveCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb );

    if ( eventinfo.type != visBase::MouseMovement ) return;

    const int sz = eventinfo.pickedobjids.size();
    bool validpicksurface = false;

    const Coord3 displayspacepos =
	SPM().getZScaleTransform()->transformBack(eventinfo.pickedpos);
    xytmousepos =
	SPM().getUTM2DisplayTransform()->transformBack(displayspacepos);

    mouseposval = mUndefValue;
    for ( int idx=0; idx<sz; idx++ )
    {
	const DataObject* pickedobj =
			    visBase::DM().getObj(eventinfo.pickedobjids[idx]);
	mDynamicCastGet(const SurveyObject*,so,pickedobj)
	if ( so )
	{
	    if ( !validpicksurface )
		validpicksurface = true;

	    if ( mIsUndefined(mouseposval) )
	    {
		const float newmouseposval = so->getValue( xytmousepos );
		if ( !mIsUndefined(newmouseposval) )
		{
		    mouseposval = newmouseposval;
		}
	    }
	    else if ( validpicksurface )
		break;
	}
    }

    if ( validpicksurface )
	mouseposchange.trigger();
}

}; // namespace visSurvey
