/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vissurvscene.cc,v 1.13 2002-04-11 14:02:26 kristofer Exp $";

#include "vissurvscene.h"
#include "visdataman.h"
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
    , appvel( 1000 )
{
    visBase::Transformation* reversedz = visBase::Transformation::create();
    addObject( reversedz );
    reversedz->setA(
	1,	0,	0,	0,
	0,	1,	0,	0,
	0,	0,	-1,	0,
	0,	0,	0,	1 );

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
	transmatrix11,	transmatrix12,	0,	0,
	transmatrix21,	transmatrix22,	0,	0,
	0,		0,		1,	0,
	0,		0,		0,	1 );

    // Set time trans
    setApparentVel( appvel );

    BinIDRange hrg = SI().range();
    StepInterval<double> vrg = SI().zRange();

    visBase::Annotation* annot = visBase::Annotation::create();
    annot->setCorner( 0, hrg.start.inl, hrg.start.crl, vrg.start );
    annot->setCorner( 1, hrg.stop.inl, hrg.start.crl, vrg.start );
    annot->setCorner( 2, hrg.stop.inl, hrg.stop.crl, vrg.start );
    annot->setCorner( 3, hrg.start.inl, hrg.stop.crl, vrg.start );
    annot->setCorner( 4, hrg.start.inl, hrg.start.crl, vrg.stop );
    annot->setCorner( 5, hrg.stop.inl, hrg.start.crl, vrg.stop );
    annot->setCorner( 6, hrg.stop.inl, hrg.stop.crl, vrg.stop );
    annot->setCorner( 7, hrg.start.inl, hrg.stop.crl, vrg.stop );

    annot->setText( 0, "In-line" );
    annot->setText( 1, "Cross-line" );
    annot->setText( 2, "TWT");
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
{ }


void visSurvey::Scene::addXYZObject( SceneObject* obj )
{
    int insertpos = getFirstIdx( timetransformation );
    if ( insertpos==size()-1) addObject(obj);
    else insertObject( insertpos, obj );
}


void visSurvey::Scene::addXYTObject( SceneObject* obj )
{
    int insertpos = getFirstIdx( inlcrltransformation );
    if ( insertpos==size()-1) addObject(obj);
    else insertObject( insertpos, obj );
}


void visSurvey::Scene::addInlCrlTObject( SceneObject* obj )
{
    addObject( obj );
}


float visSurvey::Scene::apparentVel() const { return appvel; }


void visSurvey::Scene::setApparentVel( float a )
{
    appvel = a;

    timetransformation->setA(
	1,	0,	0,		0,
	0,	1,	0,		0,
	0,	0,	appvel,		0,
	0,	0,	0,		1 );
}


