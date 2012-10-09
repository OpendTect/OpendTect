/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";


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


mVisTrans* SceneTransformManager::createZScaleTransform() const
{
    mVisTrans* tf = mVisTrans::create();
    setZScale( tf, defZStretch() );
    return tf;
}


void SceneTransformManager::setZScale( mVisTrans* tf,
				       float zscale ) const
{
    if ( !tf ) return;

    const float zsc = zscale / 2;
    tf->setA(	1,	0,	0,	0,
	    	0,	1,	0,	0,
		0,	0,	zsc,	0,
		0,	0,	0,	1 );
}


mVisTrans* 
SceneTransformManager::createUTM2DisplayTransform( const HorSampling& hs ) const
{
    mVisTrans* tf = mVisTrans::create();

    const Coord startpos = SI().transform( hs.start );
    const Coord stoppos = SI().transform( hs.stop );

    tf->setA(	1,	0,	0,	-startpos.x,
	    	0,	1,	0,	-startpos.y,
		0,	0,	-1,	0,
		0,	0,	0,	1 );
    return tf;
}


mVisTrans*
SceneTransformManager::createIC2DisplayTransform( const HorSampling& hs ) const
{
    mVisTrans* tf = mVisTrans::create();
    setIC2DispayTransform( hs, tf );
    return tf;
}


void SceneTransformManager::setIC2DispayTransform(const HorSampling& hs,
						  mVisTrans* tf ) const
{
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
    
    const int inlwidth = hs.inlRange().width();
    const int crlwidth = hs.crlRange().width();
    if ( !inlwidth )
    {
	if ( !crlwidth )
	{
	    tf->reset();
	    return;
	}
	
	x[0] = 1;
	x[1] = b[1] / crlwidth;
	x[2] = -startbid.inl - x[1] * startbid.crl;
    }
    else
	linsolver.apply( b, x );

    const double mat11 = x[0];
    const double mat12 = x[1];
    const double mat14 = x[2];

    b[0] = 0;
    b[1] = stoppos.y-startpos.y;
    b[2] = extrapos.y-startpos.y;

    if ( !crlwidth )
    {
	x[0] = b[1] / inlwidth;
	x[1] = 1;
	x[2] = -x[0] * startbid.inl - startbid.crl;
    }
    else
    	linsolver.apply( b, x );

    const double mat21 = x[0];
    const double mat22 = x[1];
    const double mat24 = x[2];

    tf->setA(	mat11,	mat12,	0,	mat14,
		mat21,	mat22,	0,	mat24,
		0,	0,	-1,	0,
		0,	0,	0,	1 );
}

} // namespace visSurvey
