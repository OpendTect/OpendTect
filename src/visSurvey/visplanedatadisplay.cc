/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visplanedatadisplay.cc,v 1.34 2003-01-24 11:31:08 nanne Exp $";

#include "visplanedatadisplay.h"

#include "attribsel.h"
#include "cubesampling.h"
#include "attribslice.h"
#include "vistexturerect.h"
#include "arrayndimpl.h"
#include "position.h"
#include "survinfo.h"
#include "visselman.h"
#include "visdataman.h"
#include "visrectangle.h"
#include "vismaterial.h"
#include "viscolortab.h"
#include "colortab.h"
#include "sorting.h"
#include "vistransform.h"
#include "iopar.h"
#include <math.h>

mCreateFactoryEntry( visSurvey::PlaneDataDisplay );

const char* visSurvey::PlaneDataDisplay::trectstr = "Texture rectangle";

visSurvey::PlaneDataDisplay::PlaneDataDisplay()
    : VisualObject(true)
    , trect(visBase::TextureRect::create())
    , cache(0)
    , as(*new AttribSelSpec)
    , cs(*new CubeSampling)
    , manipcs(*new CubeSampling)
    , moving(this)
{
    trect->ref();

    trect->getMaterial()->setAmbience( 0.8 );
    trect->getMaterial()->setDiffIntensity( 0.8 );
    trect->manipChanges()->notify( mCB(this,PlaneDataDisplay,manipChanged) );

    setType( Inline );


    trect->getRectangle().setSnapping( true );
    trect->useTexture( false );

    SPM().zscalechange.notify( mCB(this,PlaneDataDisplay,appVelChCB) );
}


void visSurvey::PlaneDataDisplay::setType(Type nt)
{
    type = nt;

    BinID startbid = SI().range().start;
    BinID stopbid = SI().range().stop;
    StepInterval<double> vrgd = SI().zRange();

    StepInterval<float> inlrange( startbid.inl,stopbid.inl,SI().inlWorkStep() );
    StepInterval<float> crlrange( startbid.crl,stopbid.crl,SI().crlWorkStep() );
    StepInterval<float> vrg( vrgd.start, vrgd.stop, vrgd.step );

    float inlspacing = SI().transform(BinID(0,0)).distance(
		       SI().transform( BinID(1,0)));
    float crlspacing = SI().transform(BinID(0,0)).distance(
		       SI().transform( BinID(0,1)));

    if ( type==Inline )
    {
	trect->getRectangle().setOrientation( visBase::Rectangle::YZ );
	trect->getRectangle().setOrigo(
		Coord3(inlrange.start, crlrange.start, vrg.start ));
	trect->getRectangle().setWidth( vrg.width(), crlrange.width() );

	trect->getRectangle().setWidthRange( 0,
		Interval<float>( vrg.width()*0.1, mUndefValue ));
	trect->getRectangle().setWidthRange( 1,
		Interval<float>( crlrange.width()*0.1, mUndefValue ));

	trect->getRectangle().setRange( 0, vrg );
	trect->getRectangle().setRange( 1, crlrange );
	trect->getRectangle().setRange( 2, inlrange );
    }
    else if ( type==Crossline )
    {
	trect->getRectangle().setOrientation( visBase::Rectangle::XZ );
	trect->getRectangle().setOrigo(
		Coord3(inlrange.start, crlrange.start, vrg.start ));
	trect->getRectangle().setWidth( inlrange.width(), vrg.width() );

	trect->getRectangle().setWidthRange( 0,
		Interval<float>( inlrange.width()*0.1, mUndefValue ));
	trect->getRectangle().setWidthRange( 1,
		Interval<float>( vrg.width()*0.1, mUndefValue ));

	trect->getRectangle().setRange( 0, inlrange );
	trect->getRectangle().setRange( 1, vrg );
	trect->getRectangle().setRange( 2, crlrange );
    }
    else
    {
	trect->getRectangle().setOrientation( visBase::Rectangle::XY );
	trect->getRectangle().setOrigo(
		Coord3(inlrange.start, crlrange.start, vrg.start ));
	trect->getRectangle().setWidth( inlrange.width(), crlrange.width() );

	trect->getRectangle().setWidthRange( 0,
		Interval<float>( inlrange.width()*0.1, mUndefValue ));
	trect->getRectangle().setWidthRange( 1,
		Interval<float>( crlrange.width()*0.1, mUndefValue ));

	trect->getRectangle().setRange( 0, inlrange );
	trect->getRectangle().setRange( 1, crlrange );
	trect->getRectangle().setRange( 2, vrg );
    }

    resetDraggerSizes( SPM().getZScale() );
}


void visSurvey::PlaneDataDisplay::setOrigo( const Coord3& pos )
{
    trect->getRectangle().setSnapping( false );
    trect->getRectangle().setOrigo( pos );
    trect->getRectangle().setSnapping( true );
}


void visSurvey::PlaneDataDisplay::setWidth( const Coord3& pos )
{
    if ( type==Inline )
	trect->getRectangle().setWidth( pos.z, pos.y );
    else if ( type==Crossline )
	trect->getRectangle().setWidth( pos.x, pos.z );
    else
	trect->getRectangle().setWidth( pos.x, pos.y );
}



void visSurvey::PlaneDataDisplay::resetDraggerSizes( float appvel )
{
    const float happvel = appvel/2;
    BinID startbid = SI().range().start;
    BinID stopbid = SI().range().stop;
    StepInterval<double> vrgd = SI().zRange();

    StepInterval<float> inlrange( startbid.inl,stopbid.inl,SI().inlWorkStep() );
    StepInterval<float> crlrange( startbid.crl,stopbid.crl,SI().crlWorkStep() );

    float inlspacing = SI().transform(BinID(0,0)).distance(
		       SI().transform( BinID(1,0)));
    float crlspacing = SI().transform(BinID(0,0)).distance(
		       SI().transform( BinID(0,1)));

    float baselength = ( crlrange.width() * crlspacing +
			 SI().zRange().width()*happvel +
			 inlrange.width() * inlspacing ) / 3;

    float draggerdiameter = 0.2 * baselength;
    float draggerlength =  baselength * 0.2;

    if ( type==Inline )
    {
	float draggertimesize = draggerdiameter / (1000*happvel);
	float draggercrlsize = draggerdiameter / crlspacing;
	float draggerinlsize = draggerlength / inlspacing;

	trect->getRectangle().setDraggerSize( draggertimesize,
					      draggercrlsize,
				   	      draggerinlsize);
    }
    else if ( type==Crossline )
    {
	float draggertimesize = draggerdiameter / (1000*happvel);
	float draggerinlsize = draggerdiameter / inlspacing;
	float draggercrlsize = draggerlength / crlspacing;

	trect->getRectangle().setDraggerSize( draggerinlsize,
					      draggertimesize,
				   	      draggercrlsize);
    }
    else
    {
	float draggerinlsize = draggerdiameter/ inlspacing;
	float draggercrlsize = draggerdiameter / crlspacing;
	float draggertimesize = draggerlength / (1000*happvel);

	trect->getRectangle().setDraggerSize( draggerinlsize,
					      draggercrlsize,
				   	      draggertimesize);
    }
}


float visSurvey::PlaneDataDisplay::calcDist( const Coord3& pos ) const
{
    const visBase::Transformation* utm2display= SPM().getUTM2DisplayTransform();
    Coord3 xytpos = utm2display->transformBack( pos );
    Coord3 planeorigo = trect->getRectangle().origo();
    float width0 = trect->getRectangle().width( 0 );
    float width1 = trect->getRectangle().width( 1 );

    BinID binid = SI().transform( Coord( xytpos.x, xytpos.y ));
    
    BinID inlcrldist( 0, 0 );
    float zdiff = 0;

    if ( type==Inline )
    {
	inlcrldist.inl = abs(binid.inl-mNINT(planeorigo.x));
	
	if ( binid.crl<planeorigo.y )
	    inlcrldist.crl = mNINT(planeorigo.y-binid.crl);
	else if ( binid.crl>planeorigo.y+width1 )
	    inlcrldist.crl = mNINT( binid.crl-planeorigo.y-width1);

	if ( xytpos.z<planeorigo.z)
	    zdiff = planeorigo.z-xytpos.z;
	else if ( xytpos.z>planeorigo.z+width0 )
	    zdiff = xytpos.z-planeorigo.z-width0;

    }
    else if ( type==Crossline )
    {
	inlcrldist.crl = abs(binid.crl-mNINT(planeorigo.y));
	
	if ( binid.inl<planeorigo.x )
	    inlcrldist.inl = mNINT(planeorigo.x-binid.inl);
	else if ( binid.inl>planeorigo.x+width0 )
	    inlcrldist.inl = mNINT( binid.inl-planeorigo.x-width0);

	if ( xytpos.z<planeorigo.z)
	    zdiff = planeorigo.z-xytpos.z;
	else if ( xytpos.z>planeorigo.z+width1 )
	    zdiff = xytpos.z-planeorigo.z-width1;

    }
    else
    {
	if ( binid.inl<planeorigo.x )
	    inlcrldist.inl = mNINT(planeorigo.x-binid.inl);
	else if ( binid.inl>planeorigo.x+width0 )
	    inlcrldist.inl = mNINT( binid.inl-planeorigo.x-width0);

	if ( binid.crl<planeorigo.y )
	    inlcrldist.crl = mNINT(planeorigo.y-binid.crl);
	else if ( binid.crl>planeorigo.y+width1 )
	    inlcrldist.crl = mNINT( binid.crl-planeorigo.y-width1);

	zdiff = (planeorigo.z - xytpos.z) * SPM().getZScale();
    }

    const float inldist =
	SI().transform( BinID(0,0)).distance( SI().transform(BinID(1,0)));
    const float crldist =
	SI().transform( BinID(0,0)).distance( SI().transform(BinID(0,1)));
    float inldiff = inlcrldist.inl * inldist;
    float crldiff = inlcrldist.crl * crldist;

    return sqrt( inldiff*inldiff + crldiff*crldiff + zdiff*zdiff );
}


void visSurvey::PlaneDataDisplay::appVelChCB( CallBacker* )
{
    resetDraggerSizes( SPM().getZScale() );
}


void visSurvey::PlaneDataDisplay::manipChanged( CallBacker* )
{
    moving.trigger();
}


void visSurvey::PlaneDataDisplay::fillPar( IOPar& par,
	TypeSet<int>& saveids ) const
{
    visBase::VisualObject::fillPar( par, saveids );
    int trectid = trect->id();
    par.set( trectstr, trectid );

    if ( saveids.indexOf( trectid )==-1 ) saveids += trectid;

    as.fillPar(par);
}


int visSurvey::PlaneDataDisplay::usePar( const IOPar& par )
{
    int res =  visBase::VisualObject::usePar( par );
    if ( res!=1 ) return res;

    int trectid;

    if ( !par.get( trectstr, trectid ))
	return -1;

    visBase::DataObject* dataobj = visBase::DM().getObj( trectid );
    if ( !dataobj ) return 0;

    mDynamicCastGet( visBase::TextureRect*, tr, dataobj );
    if ( !tr ) return -1;

    trect->unRef();
    trect = tr;
    trect->ref();

    trect->getRectangle().setSnapping( true );
    trect->useTexture( false );

    if ( trect->getRectangle().orientation() == visBase::Rectangle::YZ )
	type = Inline;
    else if ( trect->getRectangle().orientation() == visBase::Rectangle::XZ )
	type = Crossline;
    else
	type = Timeslice;

    if ( !as.usePar( par )) return -1;

    return 1;
}
    

void visSurvey::PlaneDataDisplay::showDraggers(bool yn)
{
    trect->getRectangle().displayDraggers(yn);
}


void visSurvey::PlaneDataDisplay::resetManip()
{
    trect->getRectangle().resetManip();
}


visSurvey::PlaneDataDisplay::~PlaneDataDisplay()
{
    SPM().zscalechange.remove( mCB(this,PlaneDataDisplay,appVelChCB));
    trect->selection()->remove( mCB(this,PlaneDataDisplay,select));
    trect->deSelection()->remove( mCB(this,PlaneDataDisplay,deSelect));

    trect->unRef();

    delete &as;
}


AttribSelSpec& visSurvey::PlaneDataDisplay::getAttribSelSpec()
{ return as; }

const AttribSelSpec& visSurvey::PlaneDataDisplay::getAttribSelSpec() const
{ return as; }

void visSurvey::PlaneDataDisplay::setAttribSelSpec( AttribSelSpec& as_ )
{ as = as_; }


CubeSampling& visSurvey::PlaneDataDisplay::getCubeSampling( bool manippos )
{
    CubeSampling& cubesampl = manippos ? manipcs : cs;
    cubesampl.hrg.start = BinID( mNINT(manippos
				? trect->getRectangle().manipOrigo().x
				: trect->getRectangle().origo().x),
		  	  mNINT(manippos
				? trect->getRectangle().manipOrigo().y
				: trect->getRectangle().origo().y) );

    cubesampl.hrg.stop = cubesampl.hrg.start;
    cubesampl.hrg.step = BinID( SI().inlStep(), SI().crlStep() );

    cubesampl.zrg.start = (manippos
	    ? trect->getRectangle().manipOrigo().z
	    : trect->getRectangle().origo().z);
    cubesampl.zrg.stop = cubesampl.zrg.start;

    if ( trect->getRectangle().orientation()==visBase::Rectangle::XY )
    {
	cubesampl.hrg.stop.inl += mNINT(trect->getRectangle().width( 0 ));
	cubesampl.hrg.stop.crl += mNINT(trect->getRectangle().width( 1 ));
    }
    else if ( trect->getRectangle().orientation()==visBase::Rectangle::XZ )
    {
	cubesampl.hrg.stop.inl += mNINT(trect->getRectangle().width( 0 ));
	cubesampl.zrg.stop += trect->getRectangle().width( 1 );
    }
    else
    {
	cubesampl.hrg.stop.crl += mNINT(trect->getRectangle().width(1));
	cubesampl.zrg.stop += trect->getRectangle().width(0);
    }

    return cubesampl;
}


void visSurvey::PlaneDataDisplay::setCubeSampling( const CubeSampling& cs_ )
{
    Coord3 width( cs_.hrg.stop.inl - cs_.hrg.start.inl,
			 cs_.hrg.stop.crl - cs_.hrg.start.crl, 
			 cs_.zrg.stop - cs_.zrg.start );
    setWidth( width );
    Coord3 origo(cs_.hrg.start.inl,cs_.hrg.start.crl,cs_.zrg.start);
    setOrigo( origo );
}


bool visSurvey::PlaneDataDisplay::putNewData( AttribSliceSet* sliceset )
{
    trect->setData( *(*sliceset)[0] );
    trect->useTexture( true );

    delete cache;
    cache = sliceset;

    return true;
}


const AttribSliceSet* visSurvey::PlaneDataDisplay::getPrevData() const
{
    return cache;
}


void visSurvey::PlaneDataDisplay::turnOn(bool n) { trect->turnOn(n); }


bool visSurvey::PlaneDataDisplay::isOn() const { return trect->isOn(); }


void visSurvey::PlaneDataDisplay::setColorTable( const ColorTable& ctab )
{
    trect->getColorTab().colorSeq().colors() = ctab;
    trect->getColorTab().colorSeq().colorsChanged();
}


void visSurvey::PlaneDataDisplay::setColorTable( visBase::VisColorTab* ct )
{ trect->setColorTab( ct ); }


const ColorTable& visSurvey::PlaneDataDisplay::getColorTable() const
{ return trect->getColorTab().colorSeq().colors(); }


void visSurvey::PlaneDataDisplay::setClipRate( float rate )
{ trect->setClipRate( rate ); }


float visSurvey::PlaneDataDisplay::clipRate() const
{ return trect->clipRate(); }


void visSurvey::PlaneDataDisplay::setAutoscale( bool yn )
{ trect->setAutoscale( yn ); }


bool visSurvey::PlaneDataDisplay::autoScale() const
{ return trect->autoScale(); }


void visSurvey::PlaneDataDisplay::setDataRange( const Interval<float>& intv )
{ trect->getColorTab().scaleTo( intv ); }


Interval<float> visSurvey::PlaneDataDisplay::getDataRange() const
{ return trect->getColorTab().getInterval(); }


float visSurvey::PlaneDataDisplay::getValue( const Coord3& pos ) const
{ return trect->getValue( pos ); }


void visSurvey::PlaneDataDisplay::setMaterial( visBase::Material* nm)
{ trect->setMaterial(nm); }


const visBase::Material* visSurvey::PlaneDataDisplay::getMaterial() const
{ return trect->getMaterial(); }


visBase::Material* visSurvey::PlaneDataDisplay::getMaterial()
{ return trect->getMaterial(); }


SoNode* visSurvey::PlaneDataDisplay::getData() { return trect->getData(); }


const char* visSurvey::PlaneDataDisplay::getResName( int res ) const
{
    if ( res == 1 ) return "Moderate";
    if ( res == 2 ) return "High";
    else return "Default";
}


void visSurvey::PlaneDataDisplay::setResolution( int res )
{
    trect->setResolution( res );
}


int visSurvey::PlaneDataDisplay::getResolution() const
{
    return trect->getResolution();
}

int visSurvey::PlaneDataDisplay::getNrResolutions() const
{
    return trect->getNrResolutions();
}
