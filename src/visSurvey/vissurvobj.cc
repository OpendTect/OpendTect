/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: vissurvobj.cc,v 1.1 2002-04-29 09:39:54 kristofer Exp $";

#include "vissurvobj.h"
#include "vistransform.h"
#include "survinfo.h"
#include "arrayndimpl.h"
#include "linsolv.h"



visSurvey::SurveyParamManager& visSurvey::SPM()
{
    visSurvey::SurveyParamManager* spm = 0;

    if ( !spm ) spm = new SurveyParamManager;

    return *spm;
}


visSurvey::SurveyParamManager::SurveyParamManager()
    : appvelchange( this )
    , displaytransform( visBase::Transformation::create() )
    , appveltransform( visBase::Transformation::create() )
    , inlcrltransform( visBase::Transformation::create() )
    , appvel( 0 )
{
    displaytransform->ref();
    appveltransform->ref();
    inlcrltransform->ref();

    displaytransform->setA(
			    1,      0,      0,      0,
			    0,      1,      0,      0,
			    0,      0,      -1,     0,
			    0,      0,      0,      1 );

    BinID startbid = SI().range().start;
    BinID stopbid = SI().range().stop;

    Coord startpos = SI().transform( startbid );
    Coord stoppos = SI().transform( stopbid );

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

    inlcrltransform->setA(
	    transmatrix11,	transmatrix12,	0,	transmatrix14,
	    transmatrix21,	transmatrix22,	0,	transmatrix24,
	    0,			0,		1,	0,
	    0,			0,		0,	1 );

    
    setAppVel( 2000 );
}


visSurvey::SurveyParamManager::~SurveyParamManager()
{
    displaytransform->unRef();
    appveltransform->unRef();
    inlcrltransform->unRef();
}


void visSurvey::SurveyParamManager::setAppVel( float a )
{
    if ( mIS_ZERO(appvel-a) ) return;

    appvel = a;
    appveltransform->setA(
    1,      0,      0,              0,
    0,      1,      0,              0,
    0,      0,      appvel/2,       0,
    0,      0,      0,              1 );

    appvelchange.trigger();
}


Geometry::Pos visSurvey::SurveyParamManager::coordDispl2XYT(
					const Geometry::Pos& display ) const
{
    Geometry::Pos xyz = displaytransform->transformBack( display );
    return appveltransform->transformBack( xyz );
}


Geometry::Pos visSurvey::SurveyParamManager::coordXYT2Display(
					const Geometry::Pos& xyt ) const
{
    Geometry::Pos xyz = appveltransform->transform( xyt );
    return displaytransform->transform( xyz );
}
