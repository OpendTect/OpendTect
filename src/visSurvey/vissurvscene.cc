/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vissurvscene.cc,v 1.26 2002-04-22 14:41:27 kristofer Exp $";

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


visSurvey::Scene::Scene()
    : inlcrltransformation( visBase::Transformation::create() )
    , timetransformation( visBase::Transformation::create() )
    , appvel( 0 )
    , appvelchange( this )
    , annot( 0 )
    , eventcatcher( visBase::EventCatcher::create(visBase::MouseMovement))
    , mouseposchange( this )
{
    visBase::Transformation* reversedz = visBase::Transformation::create();
    addObject( reversedz );
    reversedz->setA(
	1,	0,	0,	0,
	0,	1,	0,	0,
	0,	0,	-1,	0,
	0,	0,	0,	1 );

    addObject( eventcatcher );
    eventcatcher->eventhappened.notify( mCB( this, Scene, mouseMoveCB ));
    addObject( timetransformation );
    addObject( inlcrltransformation );

    BinID startbid = SI().range().start;
    BinID stopbid = SI().range().stop;

    Coord startpos = SI().transform( startbid );
    Coord stoppos = SI().transform( stopbid );

    // Set inlcrl transformation
    BinID firstinlinestopbid( startbid.inl, stopbid.crl );
    Coord firstinlinestoppos = SI().transform( firstinlinestopbid );

    Array2DImpl<double> A(3,3);
    A.set( 0, 0, startbid.inl );
    A.set( 0, 1, startbid.crl );
    A.set( 0, 2, 1);

    A.set( 1, 0, stopbid.inl );
    A.set( 1, 1, stopbid.crl );
    A.set( 1, 2, 1);

    A.set( 2, 0, firstinlinestopbid.inl );
    A.set( 2, 1, firstinlinestopbid.crl );
    A.set( 2, 2, 1);

    double b[] = { startpos.x, stoppos.x, firstinlinestoppos.x };
    double x[3];

    LinSolver<double> linsolver( A );
    linsolver.apply( b, x );

    double transmatrix11 = x[0];
    double transmatrix12 = x[1];
    double transmatrix14 = x[2];

    b[0] = startpos.y; b[1] = stoppos.y; b[2] = firstinlinestoppos.y;
    linsolver.apply( b, x );

    double transmatrix21 = x[0];
    double transmatrix22 = x[1];
    double transmatrix24 = x[2];

    inlcrltransformation->setA(
	transmatrix11,	transmatrix12,	0,	transmatrix14, 
	transmatrix21,	transmatrix22,	0,	transmatrix24,
	0,		0,		1,	0,
	0,		0,		0,	1 );

    xoffset = 0;
    yoffset = 0;

    // Set time trans
    setApparentVel( 2000 );

    BinIDRange hrg = SI().range();
    StepInterval<double> vrg = SI().zRange();

    annot = visBase::Annotation::create();
    BinID c0( hrg.start.inl, hrg.start.crl ); Coord coord0=SI().transform( c0 );
    BinID c1( hrg.stop.inl, hrg.start.crl ); Coord coord1=SI().transform( c1 );
    BinID c2( hrg.stop.inl, hrg.stop.crl ); Coord coord2=SI().transform( c2 );
    BinID c3( hrg.start.inl, hrg.stop.crl ); Coord coord3=SI().transform( c3 );

    annot->setCorner( 0, coord0.x, coord0.y, vrg.start );
    annot->setCorner( 1, coord1.x, coord1.y, vrg.start );
    annot->setCorner( 2, coord2.x, coord2.y, vrg.start );
    annot->setCorner( 3, coord3.x, coord3.y, vrg.start );
    annot->setCorner( 4, coord0.x, coord0.y, vrg.stop );
    annot->setCorner( 5, coord1.x, coord1.y, vrg.stop );
    annot->setCorner( 6, coord2.x, coord2.y, vrg.stop );
    annot->setCorner( 7, coord3.x, coord3.y, vrg.stop );

    annot->setText( 0, "In-line" );
    annot->setText( 1, "Cross-line" );
    annot->setText( 2, SI().zIsTime() ? "TWT" : "Depth" );
    addXYTObject( annot );

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


float visSurvey::Scene::apparentVel() const { return appvel; }


void visSurvey::Scene::setApparentVel( float a )
{
    if ( mIS_ZERO(appvel-a) ) return;
    appvel = a;

    timetransformation->setA(
	1,	0,	0,		0,
	0,	1,	0,		0,
	0,	0,	appvel/2,	0,
	0,	0,	0,		1 );

    appvelchange.trigger();
}


void visSurvey::Scene::showAnnotText( bool yn )
{
    annot->showText( yn );
}


bool  visSurvey::Scene::isAnnotTextShown() const
{
    return annot->isTextShown();
}


Geometry::Pos visSurvey::Scene::getRealCoord(Geometry::Pos display) const
{
    Geometry::Pos res;
    res.x = display.x + xoffset;
    res.y = display.y + yoffset;
    res.z = display.z;

    return res;
}


Geometry::Pos visSurvey::Scene::getDisplayCoord(Geometry::Pos real) const
{
    Geometry::Pos res;
    res.x = real.x - xoffset;
    res.y = real.y - yoffset;
    res.z = real.z;

    return res;
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

    xytmousepos = getRealCoord(eventinfo.pickedpos);
    xytmousepos.z /= -apparentVel();
    xytmousepos.z *= 2;

    Geometry::Pos inlcrl = xytmousepos;
    BinID binid = SI().transform( Coord( xytmousepos.x, xytmousepos.y ));
    inlcrl.x = binid.inl;
    inlcrl.y = binid.crl;

    mouseposval = sd->getValue( inlcrl );
    mouseposchange.trigger();
}
