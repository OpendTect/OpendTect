/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: vistransmgr.cc,v 1.6 2009-07-22 16:01:46 cvsbert Exp $";


#include "vistransmgr.h"
#include "vissurvscene.h"
#include "vistransform.h"
#include "survinfo.h"
#include "arrayndimpl.h"
#include "cubesampling.h"
#include "linsolv.h"


namespace visSurvey
{

SceneTransformManager& STM()
{
    static SceneTransformManager* tm = 0;
    if ( !tm ) mTryAlloc( tm, SceneTransformManager );
    return *tm;
}


visBase::Transformation* SceneTransformManager::createZScaleTransform()
{
    visBase::Transformation* tf = visBase::Transformation::create();
    setZScale( tf, defZStretch() );
    return tf;
}


void SceneTransformManager::setZScale( visBase::Transformation* tf,
				       float zscale )
{
    if ( !tf ) return;

    const float zsc = zscale / 2;
    tf->setA(	1,	0,	0,	0,
	    	0,	1,	0,	0,
		0,	0,	zsc,	0,
		0,	0,	0,	1 );
}


visBase::Transformation* 
    SceneTransformManager::createUTM2DisplayTransform( const HorSampling& hs )
{
    visBase::Transformation* tf = visBase::Transformation::create();

    const Coord startpos = SI().transform( hs.start );
    const Coord stoppos = SI().transform( hs.stop );

    tf->setA(	1,	0,	0,	-startpos.x,
	    	0,	1,	0,	-startpos.y,
		0,	0,	-1,	0,
		0,	0,	0,	1 );
    return tf;
}


visBase::Transformation*
    SceneTransformManager::createIC2DisplayTransform( const HorSampling& hs )
{
    visBase::Transformation* tf = visBase::Transformation::create();

    const BinID startbid = hs.start;
    const BinID stopbid = hs.stop;
    const BinID extrabid( startbid.inl, stopbid.crl );

    const Coord startpos = SI().transform( startbid );
    const Coord stoppos = SI().transform( stopbid );
    const Coord extrapos = SI().transform( extrabid );

    Array2DImpl<double> A(3,3);
    A.set( 0, 0, startbid.inl );
    A.set( 0, 1, startbid.crl );
    A.set( 0, 2, 1);

    A.set( 1, 0, stopbid.inl );
    A.set( 1, 1, stopbid.crl );
    A.set( 1, 2, 1);

    A.set( 2, 0, extrabid.inl );
    A.set( 2, 1, extrabid.crl );
    A.set( 2, 2, 1);

    double b[] = { 0, stoppos.x-startpos.x, extrapos.x-startpos.x };
    double x[3];

    LinSolver<double> linsolver( A );
    linsolver.apply( b, x );
    double mat11 = x[0];
    double mat12 = x[1];
    double mat14 = x[2];

    b[0] = 0;
    b[1] = stoppos.y-startpos.y;
    b[2] = extrapos.y-startpos.y;
    linsolver.apply( b, x );

    double mat21 = x[0];
    double mat22 = x[1];
    double mat24 = x[2];

    tf->setA(	mat11,	mat12,	0,	mat14,
		mat21,	mat22,	0,	mat24,
		0,	0,	-1,	0,
		0,	0,	0,	1 );
    return tf;
}

} // namespace visSurvey
