/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vissurvscene.cc,v 1.28 2002-04-29 09:27:44 kristofer Exp $";

#include "vissurvscene.h"
#include "visplanedatadisplay.h"
#include "visdataman.h"
#include "visevent.h"
#include "vistransform.h"
#include "position.h"
#include "survinfo.h"
#include "linsolv.h"
#include "visannot.h"
#include "vislight.h"

#include <limits.h>

mCreateFactoryEntry( visSurvey::Scene );


visSurvey::Scene::Scene()
    : inlcrltransformation( SPM().getInlCrlTransform() )
    , timetransformation( SPM().getAppvelTransform() )
    , annot( 0 )
    , eventcatcher( visBase::EventCatcher::create(visBase::MouseMovement))
    , mouseposchange( this )
{
    addObject( const_cast<visBase::Transformation*>(
						SPM().getDisplayTransform()));
    addObject( eventcatcher );
    eventcatcher->eventhappened.notify( mCB( this, Scene, mouseMoveCB ));
    addObject( const_cast<visBase::Transformation*>(timetransformation) );
    addObject( const_cast<visBase::Transformation*>(inlcrltransformation) );

    BinIDRange hrg = SI().range();
    StepInterval<double> vrg = SI().zRange();

    annot = visBase::Annotation::create();
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

    annot->setText( 0, "In-line" );
    annot->setText( 1, "Cross-line" );
    annot->setText( 2, SI().zIsTime() ? "TWT" : "Depth" );
    addInlCrlTObject( annot );

    visBase::DirectionalLight* light = visBase::DirectionalLight::create();
    light->setDirection( 0, 0, 1 );
    addXYZObject( light );

    light = visBase::DirectionalLight::create();
    light->setDirection( 0, 0, -1 );
    addXYZObject( light );

    light = visBase::DirectionalLight::create();
    light->setDirection( 0, 1, 0 );
    addXYZObject( light );

    light = visBase::DirectionalLight::create();
    light->setDirection( 0,-1, 0 );
    addXYZObject( light );

    light = visBase::DirectionalLight::create();
    light->setDirection( 1, 0, 0 );
    addXYZObject( light );

    light = visBase::DirectionalLight::create();
    light->setDirection(-1, 0, 0 );
    addXYZObject( light );
}


visSurvey::Scene::~Scene()
{
    eventcatcher->eventhappened.remove( mCB( this, Scene, mouseMoveCB ));
}


void visSurvey::Scene::addXYZObject( SceneObject* obj )
{
    int insertpos = getFirstIdx( timetransformation );
    insertObject( insertpos, obj );
}


void visSurvey::Scene::addXYTObject( SceneObject* obj )
{
    int insertpos = getFirstIdx( inlcrltransformation );
    insertObject( insertpos, obj );
}


void visSurvey::Scene::addInlCrlTObject( SceneObject* obj )
{
    addObject( obj );
}


void visSurvey::Scene::showAnnotText( bool yn )
{
    annot->showText( yn );
}


bool  visSurvey::Scene::isAnnotTextShown() const
{
    return annot->isTextShown();
}


Geometry::Pos visSurvey::Scene::getMousePos( bool xyt ) const
{
   if ( xyt ) return xytmousepos;
   
   Geometry::Pos res = xytmousepos;
   BinID binid = SI().transform( Coord( res.x, res.y ));

   res.x = binid.inl;
   res.y = binid.crl;
   return res;
}


void visSurvey::Scene::mouseMoveCB(CallBacker* cb )
{
    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb );

    if ( eventinfo.type != visBase::MouseMovement ) return;

    const int sz = eventinfo.pickedobjids.size();
    bool validpicksurface = false;
    const visSurvey::PlaneDataDisplay* sd = 0;

    for ( int idx=0; idx<sz; idx++ )
    {
	const DataObject* pickedobj =
			    visBase::DM().getObj(eventinfo.pickedobjids[idx]);

	if ( typeid(*pickedobj)==typeid(visSurvey::PlaneDataDisplay) )
	{
	    validpicksurface = true;
	    sd = (const visSurvey::PlaneDataDisplay*) pickedobj;
	    break;
	}
    }

    if ( !validpicksurface ) return;

    xytmousepos = SPM().coordDispl2XYT(eventinfo.pickedpos);

    Geometry::Pos inlcrl = xytmousepos;
    BinID binid = SI().transform( Coord( xytmousepos.x, xytmousepos.y ));
    inlcrl.x = binid.inl;
    inlcrl.y = binid.crl;

    mouseposval = sd->getValue( inlcrl );
    mouseposchange.trigger();
}
