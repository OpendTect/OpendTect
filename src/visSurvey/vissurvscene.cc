/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vissurvscene.cc,v 1.4 2002-02-28 07:16:57 kristofer Exp $";

#include "vissurvscene.h"
#include "vistransform.h"
#include "position.h"
#include "survinfo.h"
#include "linsolv.h"
#include "visannot.h"
#include "vislight.h"

#include <limits.h>


int visSurvey::Scene::xytidoffset = INT_MAX >> 2;
int visSurvey::Scene::inlcrloffset = INT_MAX >> 1;
// const float visSurvey::Scene::defvel = 1000;

visSurvey::Scene::Scene()
    : xytworld( new visBase::SceneObjectGroup(true) )
    , inlcrlworld( new visBase::SceneObjectGroup(true) )
    , inlcrltransformation( new visBase::Transformation )
    , xytranslation( new visBase::Transformation )
    , timetransformation( new visBase::Transformation )
    , appvel( 1000 )
{
    addObject( xytranslation );
    addObject( xytworld );
    xytworld->addObject( timetransformation );
    xytworld->addObject( inlcrlworld );
    inlcrlworld->addObject( inlcrltransformation );

    // Set xytranslation
    BinID startbid = SI().range().start;
    BinID stopbid = SI().range().stop;

    Coord startpos = SI().transform( startbid );
    Coord stoppos = SI().transform( stopbid );

    inlcrltransformation->setA(
	1,	0,	0,	-startpos.x,
	0,	1,	0,	-startpos.y,
	0,	0,	1,	0,
	0,	0,	0,	1 );


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

    timetransformation->setA(
	1,	0,	0,		0,
	0,	1,	0,		0,
	0,	0,	appvel,		0,
	0,	0,	0,		1 );

    BinIDRange hrg = SI().range();
    StepInterval<double> vrg = SI().zRange();

    visBase::Annotation* annot = new visBase::Annotation( *this );
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

    visBase::DirectionalLight* light = new visBase::DirectionalLight;
    light->setDirection( 0, 0, 1 );
    addXYZObject( light );

    light = new visBase::DirectionalLight;
    light->setDirection( 0, 0, -1 );
    addXYZObject( light );

    light = new visBase::DirectionalLight;
    light->setDirection( 0, 1, 0 );
    addXYZObject( light );

    light = new visBase::DirectionalLight;
    light->setDirection( 0,-1, 0 );
    addXYZObject( light );

    light = new visBase::DirectionalLight;
    light->setDirection( 1, 0, 0 );
    addXYZObject( light );

    light = new visBase::DirectionalLight;
    light->setDirection(-1, 0, 0 );
    addXYZObject( light );
}


visSurvey::Scene::~Scene()
{ }


int visSurvey::Scene::addXYZObject( SceneObject* obj )
{
    return addObject( obj );
}


int visSurvey::Scene::addXYTObject( SceneObject* obj )
{
    return xytidoffset + xytworld->addObject(obj);
}


int visSurvey::Scene::addInlCrlTObject( SceneObject* obj )
{
    return inlcrloffset + inlcrlworld->addObject( obj );
}


void visSurvey::Scene::removeObject( int id )
{
    if ( id >=inlcrloffset )
	inlcrlworld->removeObject( id-inlcrloffset );
    else if ( id>=xytidoffset )
	xytworld->removeObject( id-xytidoffset);
    else
	 visBase::SceneObjectGroup::removeObject( id );
}



