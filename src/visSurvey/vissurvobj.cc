/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: vissurvobj.cc,v 1.15 2004-04-29 07:02:34 kristofer Exp $";

#include "vissurvobj.h"

#include "arrayndimpl.h"
#include "attribslice.h"
#include "iopar.h"
#include "linsolv.h"
#include "seistrc.h"
#include "survinfo.h"
#include "visdataman.h"
#include "vistransform.h"


const char* visSurvey::SurveyParamManager::zscalestr = "Z Scale";
float visSurvey::SurveyParamManager::defzscale = 2;


void visSurvey::SurveyObject::getChildren( TypeSet<int>& ) const
{}


bool visSurvey::SurveyObject::canDuplicate() const
{ return false; }


visSurvey::SurveyObject* visSurvey::SurveyObject::duplicate() const
{ return 0; }


void visSurvey::SurveyObject::showManipulator(bool yn) {}


bool visSurvey::SurveyObject::isManipulatorShown() const
{ return false; }


bool visSurvey::SurveyObject::isManipulated() const
{ return false; }


bool visSurvey::SurveyObject::canResetManipulation() const
{ return false; }


void visSurvey::SurveyObject::resetManipulation()
{  }


void visSurvey::SurveyObject::acceptManipulation()
{  }


bool visSurvey::SurveyObject::hasColor() const
{ return false; }


bool visSurvey::SurveyObject::hasMaterial() const
{ return false; }


int visSurvey::SurveyObject::nrResolutions() const
{ return 1; }


BufferString visSurvey::SurveyObject::getResolutionName(int idx) const
{ return BufferString(idx); }


int visSurvey::SurveyObject::getResolution() const
{ return 0; }


void visSurvey::SurveyObject::setResolution(int)
{}



int visSurvey::SurveyObject::getAttributeFormat() const
{ return -1; }


bool visSurvey::SurveyObject::hasColorAttribute() const
{ return false; }


const AttribSelSpec* visSurvey::SurveyObject::getSelSpec() const
{ return 0; }


const ColorAttribSel* visSurvey::SurveyObject::getColorSelSpec() const
{ return 0; }


void visSurvey::SurveyObject::setSelSpec( const AttribSelSpec& )
{}


void visSurvey::SurveyObject::setColorSelSpec( const ColorAttribSel& )
{}


const TypeSet<float>* visSurvey::SurveyObject::getHistogram() const
{ return 0; }


int visSurvey::SurveyObject::getColTabID() const { return -1; }


CubeSampling visSurvey::SurveyObject::getCubeSampling() const
{
    CubeSampling cs;
    return cs;
} 


void visSurvey::SurveyObject::setCubeSampling( CubeSampling )
{}


bool visSurvey::SurveyObject::setDataVolume( bool, AttribSliceSet* slc)
{ delete slc; return true; }


const AttribSliceSet* visSurvey::SurveyObject::getCacheVolume( bool ) const
{ return 0; }


void visSurvey::SurveyObject::getDataTraceBids(TypeSet<BinID>&) const
{}


Interval<float> visSurvey::SurveyObject::getDataTraceRange() const
{ return Interval<float>(0,0); }


void visSurvey::SurveyObject::setTraceData( bool, ObjectSet<SeisTrc>* trcs)
{ if ( trcs ) deepErase( *trcs ); }


void visSurvey::SurveyObject::getDataRandomPos(
	ObjectSet< TypeSet<BinIDZValues> >&) const
{}


void visSurvey::SurveyObject::setRandomPosData(bool,
	const ObjectSet<const TypeSet<const BinIDZValues> >* vals )
{}


visSurvey::SurveyParamManager& visSurvey::SPM()
{
    static visSurvey::SurveyParamManager* spm = 0;

    if ( !spm ) spm = new SurveyParamManager;

    return *spm;
}


visSurvey::SurveyParamManager::SurveyParamManager()
    : zscalechange( this )
    , utm2displaytransform( 0 )
    , zscaletransform( 0 )
    , inlcrl2displaytransform( 0 )
    , zscale( 0 )
{
    visBase::DM().removeallnotify.notify(
	    mCB(this,SurveyParamManager,removeTransforms) );
}


visSurvey::SurveyParamManager::~SurveyParamManager()
{
    if ( utm2displaytransform ) utm2displaytransform->unRef();
    if ( zscaletransform ) zscaletransform->unRef();
    if ( inlcrl2displaytransform ) inlcrl2displaytransform->unRef();

    visBase::DM().removeallnotify.remove(
	    mCB( this, SurveyParamManager, removeTransforms ));
}


void visSurvey::SurveyParamManager::setZScale( float a )
{
    if ( !utm2displaytransform ) createTransforms();

    if ( mIS_ZERO(zscale-a) ) return;

    zscale = a;
    zscaletransform->setA(
    1,      0,      0,              0,
    0,      1,      0,              0,
    0,      0,      zscale/2,       0,
    0,      0,      0,              1 );

    zscalechange.trigger();
}


float visSurvey::SurveyParamManager::getZScale() const
{ return utm2displaytransform ? zscale : defzscale; }


visBase::Transformation*
visSurvey::SurveyParamManager::getUTM2DisplayTransform()
{
    if ( !utm2displaytransform )
	createTransforms();

    return utm2displaytransform;
}


visBase::Transformation*
visSurvey::SurveyParamManager::getZScaleTransform()
{
    if ( !utm2displaytransform )
	createTransforms();

    return zscaletransform;
}


visBase::Transformation*
visSurvey::SurveyParamManager::getInlCrl2DisplayTransform()
{
    if ( !utm2displaytransform )
	createTransforms();

    return inlcrl2displaytransform;
}


void visSurvey::SurveyParamManager::createTransforms()
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


void visSurvey::SurveyParamManager::removeTransforms(CallBacker*)
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
