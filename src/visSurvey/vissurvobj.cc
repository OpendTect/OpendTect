/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: vissurvobj.cc,v 1.23 2004-07-16 15:35:26 bert Exp $";

#include "vissurvobj.h"

#include "arrayndimpl.h"
#include "attribslice.h"
#include "iopar.h"
#include "linsolv.h"
#include "seisbuf.h"
#include "survinfo.h"
#include "visdataman.h"
#include "vistransform.h"

namespace visSurvey {

float SurveyObject::sDefMaxDist = 10;

const char* SurveyParamManager::zscalestr = "Z Scale";
float SurveyParamManager::defzscale = 2;


bool SurveyObject::setDataVolume( bool color, AttribSliceSet* slc )
{
    delete slc;
    return true;
}


void SurveyObject::setTraceData( bool color, SeisTrcBuf& trcs )
{
    trcs.deepErase();
}


SurveyParamManager& SPM()
{
    static SurveyParamManager* spm = 0;

    if ( !spm ) spm = new SurveyParamManager;

    return *spm;
}


SurveyParamManager::SurveyParamManager()
    : zscalechange( this )
    , utm2displaytransform( 0 )
    , zscaletransform( 0 )
    , inlcrl2displaytransform( 0 )
    , zscale( 0 )
{
    visBase::DM().removeallnotify.notify(
	    mCB(this,SurveyParamManager,removeTransforms) );
}


SurveyParamManager::~SurveyParamManager()
{
    if ( utm2displaytransform ) utm2displaytransform->unRef();
    if ( zscaletransform ) zscaletransform->unRef();
    if ( inlcrl2displaytransform ) inlcrl2displaytransform->unRef();

    visBase::DM().removeallnotify.remove(
	    mCB( this, SurveyParamManager, removeTransforms ));
}


void SurveyParamManager::setZScale( float a )
{
    if ( !utm2displaytransform ) createTransforms();

    if ( mIsEqual(zscale,a,mDefEps) ) return;

    zscale = a;
    zscaletransform->setA(
    1,      0,      0,              0,
    0,      1,      0,              0,
    0,      0,      zscale/2,       0,
    0,      0,      0,              1 );

    zscalechange.trigger();
}


float SurveyParamManager::getZScale() const
{ return utm2displaytransform ? zscale : defzscale; }


visBase::Transformation* SurveyParamManager::getUTM2DisplayTransform()
{
    if ( !utm2displaytransform )
	createTransforms();

    return utm2displaytransform;
}


visBase::Transformation* SurveyParamManager::getZScaleTransform()
{
    if ( !utm2displaytransform )
	createTransforms();

    return zscaletransform;
}


visBase::Transformation* SurveyParamManager::getInlCrl2DisplayTransform()
{
    if ( !utm2displaytransform )
	createTransforms();

    return inlcrl2displaytransform;
}


void SurveyParamManager::createTransforms()
{
    utm2displaytransform = visBase::Transformation::create();
    zscaletransform = visBase::Transformation::create();
    inlcrl2displaytransform = visBase::Transformation::create();

    utm2displaytransform->ref();
    zscaletransform->ref();
    inlcrl2displaytransform->ref();

    BinID startbid = SI().range().start;
    BinID stopbid = SI().range().stop;
    BinID firstinlinestopbid( startbid.inl, stopbid.crl );

    Coord startpos = SI().transform( startbid );
    Coord stoppos = SI().transform( stopbid );
    Coord firstinlinestoppos = SI().transform( firstinlinestopbid );
    const float zfactor = SI().zFactor();

    utm2displaytransform->setA(
			    1,      0,      0,      -startpos.x,
			    0,      1,      0,      -startpos.y,
			    0,      0,      -zfactor,  0,
			    0,      0,      0,      1 );

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

    double b[] = { 0, stoppos.x-startpos.x, firstinlinestoppos.x-startpos.x };
    double x[3];

    LinSolver<double> linsolver( A );
    linsolver.apply( b, x );
    double transmatrix11 = x[0];
    double transmatrix12 = x[1];
    double transmatrix14 = x[2];

    b[0] = 0;
    b[1] = stoppos.y-startpos.y;
    b[2] = firstinlinestoppos.y-startpos.y;
    linsolver.apply( b, x );

    double transmatrix21 = x[0];
    double transmatrix22 = x[1];
    double transmatrix24 = x[2];

    inlcrl2displaytransform->setA(
	    transmatrix11,	transmatrix12,	0,	transmatrix14,
	    transmatrix21,	transmatrix22,	0,	transmatrix24,
	    0,			0,		-zfactor,	0,
	    0,			0,		0,	1 );

    
    float zsc = defzscale;
    SI().pars().get( zscalestr, zsc );
    setZScale( zsc );
}


void SurveyParamManager::removeTransforms(CallBacker*)
{
    if ( utm2displaytransform )
    {
	utm2displaytransform->unRef();
	utm2displaytransform=0;
    }
    if ( zscaletransform )
    {
	zscaletransform->unRef();
	zscaletransform=0;
    }
    if ( inlcrl2displaytransform )
    {
	inlcrl2displaytransform->unRef();
	inlcrl2displaytransform=0;
    }

    zscale = 1;
}

}; // namespace visSurvey
