/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vissurvscene.cc,v 1.1 2002-02-26 20:19:33 kristofer Exp $";

#include "vissurvscene.h"
#include "vistransform.h"
#include "position.h"
#include "survinfo.h"
#include "linsolv.h"

#include <limits.h>


int visSurvey::SurveyScene::xytidoffset = INT_MAX >> 2;
int visSurvey::SurveyScene::inlcrloffset = INT_MAX >> 1;
float visSurvey::SurveyScene::defvel = 1000;

visSurvey::SurveyScene::SurveyScene()
    : xytworld( new visBase::SceneObjectGroup(true) )
    , inlcrlworld( new visBase::SceneObjectGroup(true) )
    , inlcrltransformation( new visBase::Transformation )
    , xytranslation( new visBase::Transformation )
    , timetransformation( new visBase::Transformation )
    , appvel( defvel )
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
	0,	0,	appvel/defvel,	0,
	0,	0,	0,		1 );
}


visSurvey::SurveyScene::~SurveyScene()
{ }


int visSurvey::SurveyScene::addXYZObject( SceneObject* obj )
{
    return addObject( obj );
}


int visSurvey::SurveyScene::addXYTObject( SceneObject* obj )
{
    return xytidoffset + xytworld->addObject(obj);
}


int visSurvey::SurveyScene::addInlCrlTObject( SceneObject* obj )
{
    return inlcrloffset + inlcrlworld->addObject( obj );
}


void visSurvey::SurveyScene::removeObject( int id )
{
    if ( id >=inlcrloffset )
	inlcrlworld->removeObject( id-inlcrloffset );
    else if ( id>=xytidoffset )
	xytworld->removeObject( id-xytidoffset);
    else
	 visBase::SceneObjectGroup::removeObject( id );
}



