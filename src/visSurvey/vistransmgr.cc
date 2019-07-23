/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2005
________________________________________________________________________

-*/


#include "vistransmgr.h"
#include "vissurvscene.h"
#include "vistransform.h"
#include "survgeom3d.h"
#include "survinfo.h"
#include "arrayndimpl.h"
#include "trckeysampling.h"
#include "linsolv.h"


namespace visSurvey
{

SceneTransformManager& STM()
{
    mDefineStaticLocalObject( SceneTransformManager*, tm,
			      = new SceneTransformManager );
    return *tm;
}

#define mComputeZTranslation( sign ) (-1*sign*zfactor*zmidpt)


void SceneTransformManager::computeUTM2DisplayTransform(
	const SurvGeom3D& sg, float zfactor, float zmidpt,
        mVisTrans* res)
{
    const Coord startpos = SI().transform( sg.origin() );

    const float ztransl = mComputeZTranslation( 1 );

    res->setA(	1,	0,	0,		-startpos.x_,
		0,	1,	0,		-startpos.y_,
		0,	0,	zfactor,	ztransl,
		0,	0,	0,		1 );
}


void SceneTransformManager::computeICRotationTransform(
	const SurvGeom3D& sg, float zfactor, float zmidpt,
		visBase::Transformation* rotation,
		visBase::Transformation* disptrans )
{
    const TrcKeySampling hs( true );

    const BinID startbid = hs.start_;
    const BinID stopbid = hs.stop_;
    const BinID extrabid( startbid.inl(), stopbid.crl() );

    const Coord startpos = SI().transform( startbid );
    const Coord stoppos = SI().transform( stopbid );
    const Coord extrapos = SI().transform( extrabid );

    const float inldist = sg.inlDistance();
    const float crldist = sg.crlDistance();

    Array2DImpl<double> A(3,3);
    A.set( 0, 0, startbid.inl()*inldist );
    A.set( 0, 1, startbid.crl()*crldist );
    A.set( 0, 2, 1);

    A.set( 1, 0, stopbid.inl()*inldist );
    A.set( 1, 1, stopbid.crl()*crldist );
    A.set( 1, 2, 1);

    A.set( 2, 0, extrabid.inl()*inldist );
    A.set( 2, 1, extrabid.crl()*crldist );
    A.set( 2, 2, 1);

    double b[] = { 0, stoppos.x_-startpos.x_, extrapos.x_-startpos.x_ };
    double x[3];

    LinSolver<double> linsolver( A );
    if ( !linsolver.init(SilentTaskRunnerProvider()) )
	return;

    const int inlwidth = hs.inlRange().width();
    const int crlwidth = hs.crlRange().width();

    if ( !inlwidth )
    {
	if ( !crlwidth )
	{
	    rotation->reset();
	    return;
	}

	x[0] = 1;
	x[1] = b[1] / (crlwidth*crldist);
	x[2] = -startbid.inl()*inldist - x[1] * startbid.crl()*crldist;
    }
    else
	linsolver.apply( b, x );

    const double mat11 = x[0];
    const double mat12 = x[1];
    const double mat14 = x[2];

    b[0] = 0;
    b[1] = stoppos.y_-startpos.y_;
    b[2] = extrapos.y_-startpos.y_;

    if ( !crlwidth )
    {
	x[0] = b[1] / (inlwidth*inldist);
	x[1] = 1;
	x[2] = -x[0] * startbid.inl()*inldist - startbid.crl()*crldist;
    }
    else
	linsolver.apply( b, x );

    const double mat21 = x[0];
    const double mat22 = x[1];
    const double mat24 = x[2];

    const int sign = sg.isRightHandSystem() ? -1 : 1;

    rotation->setA(	mat11,	mat12,	0,	mat14,
			mat21,	mat22,	0,	mat24,
			0,	0,	sign,	0,
			0,	0,	0,	1 );


    const float ztransl = mComputeZTranslation(sign);

    if ( disptrans )
	disptrans->setA( inldist,	0,		0,		0,
			 0,		crldist,	0,		0,
			 0,		0,		sign*zfactor,	ztransl,
			 0,		0,		0,		1 );
}

} // namespace visSurvey
