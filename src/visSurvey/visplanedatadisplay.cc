/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visplanedatadisplay.cc,v 1.19 2002-07-08 05:43:03 kristofer Exp $";

#include "visplanedatadisplay.h"
#include "geompos.h"
#include "cubesampling.h"
#include "attribsel.h"
#include "attribslice.h"
#include "vistexturerect.h"
#include "arrayndimpl.h"
#include "position.h"
#include "survinfo.h"
#include "visselman.h"
#include "visdataman.h"
#include "visrectangle.h"
#include "vismaterial.h"
#include "sorting.h"
#include "iopar.h"

mCreateFactoryEntry( visSurvey::PlaneDataDisplay );

const char* visSurvey::PlaneDataDisplay::trectstr = "Texture rectangle";

visSurvey::PlaneDataDisplay::PlaneDataDisplay()
    : VisualObject( true )
    , trect( visBase::TextureRect::create() )
    , selected_( false )
    , cs(*new CubeSampling)
    , as(*new AttribSelSpec)
    , moving( this )
{
    trect->ref();
    selection()->notify( mCB(this,PlaneDataDisplay,select));
    deSelection()->notify( mCB(this,PlaneDataDisplay,deSelect));

    trect->getMaterial()->setAmbience( 0.6 );
    trect->getMaterial()->setDiffIntensity( 0.8 );

    setType( Inline );


    trect->getRectangle().setSnapping( true );
    trect->useTexture( false );

    SPM().appvelchange.notify(  mCB(this,PlaneDataDisplay,appVelChCB));
}


void visSurvey::PlaneDataDisplay::setType(Type nt)
{
    type = nt;

    BinID startbid = SI().range().start;
    BinID stopbid = SI().range().stop;
    StepInterval<double> vrgd = SI().zRange();

    StepInterval<float> inlrange( startbid.inl, stopbid.inl, SI().step().inl );
    StepInterval<float> crlrange( startbid.crl, stopbid.crl, SI().step().crl );
    StepInterval<float> vrg( vrgd.start, vrgd.stop, vrgd.step );

    float inlspacing = SI().transform(BinID(0,0)).distance(
		       SI().transform( BinID(1,0)));
    float crlspacing = SI().transform(BinID(0,0)).distance(
		       SI().transform( BinID(0,1)));

    if ( type==Inline )
    {
	trect->getRectangle().setOrientation( visBase::Rectangle::YZ );
	trect->getRectangle().setOrigo(
		Geometry::Pos(inlrange.start, crlrange.start, vrg.start ));
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
		Geometry::Pos(inlrange.start, crlrange.start, vrg.start ));
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
		Geometry::Pos(inlrange.start, crlrange.start, vrg.start ));
	trect->getRectangle().setWidth( inlrange.width(), crlrange.width() );

	trect->getRectangle().setWidthRange( 0,
		Interval<float>( inlrange.width()*0.1, mUndefValue ));
	trect->getRectangle().setWidthRange( 1,
		Interval<float>( crlrange.width()*0.1, mUndefValue ));

	trect->getRectangle().setRange( 0, inlrange );
	trect->getRectangle().setRange( 1, crlrange );
	trect->getRectangle().setRange( 2, vrg );
    }

    resetDraggerSizes( SPM().getAppVel() );
}


void visSurvey::PlaneDataDisplay::setOrigo( const Geometry::Pos& pos )
{
    trect->getRectangle().setSnapping( false );
    trect->getRectangle().setOrigo( pos );
    trect->getRectangle().setSnapping( true );
}


void visSurvey::PlaneDataDisplay::setWidth( const Geometry::Pos& pos )
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

    StepInterval<float> inlrange( startbid.inl, stopbid.inl, SI().step().inl );
    StepInterval<float> crlrange( startbid.crl, stopbid.crl, SI().step().crl );

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
	float draggertimesize = draggerdiameter/happvel;
	float draggercrlsize = draggerdiameter / crlspacing;
	float draggerinlsize = draggerlength / inlspacing;

	trect->getRectangle().setDraggerSize( draggertimesize,
					      draggercrlsize,
				   	      draggerinlsize);
    }
    else if ( type==Crossline )
    {
	float draggertimesize = draggerdiameter/happvel;
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
	float draggertimesize = draggerlength / happvel;

	trect->getRectangle().setDraggerSize( draggerinlsize,
					      draggercrlsize,
				   	      draggertimesize);
    }
}


float visSurvey::PlaneDataDisplay::calcDist( const Geometry::Pos& pos ) const
{
    Geometry::Pos xytpos = SPM().coordDispl2XYT( pos );
    Geometry::Pos planeorigo = trect->getRectangle().origo();
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

	zdiff = (planeorigo.z - xytpos.z) * SPM().getAppVel();
    }

    const float inldist =
	SI().transform( BinID(0,0)).distance( SI().transform(BinID(1,0)));
    const float crldist =
	SI().transform( BinID(0,0)).distance( SI().transform(BinID(0,1)));
    float inldiff = inlcrldist.inl * inldist;
    float crldiff = inlcrldist.crl * crldist;

    return sqrt( inldiff*inldiff + crldiff*crldiff + zdiff*zdiff );
}


void visSurvey::PlaneDataDisplay::appVelChCB( CallBacker* cb )
{
    resetDraggerSizes( SPM().getAppVel() );
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
    SPM().appvelchange.remove( mCB(this,PlaneDataDisplay,appVelChCB));
    trect->selection()->remove( mCB(this,PlaneDataDisplay,select));
    trect->deSelection()->remove( mCB(this,PlaneDataDisplay,deSelect));

    trect->unRef();

    delete &cs;
    delete &as;
}


bool visSurvey::PlaneDataDisplay::updateAtNewPos()
{
    succeeded_ = false;
    moving.trigger();
    return succeeded_;
}

AttribSelSpec& visSurvey::PlaneDataDisplay::getAttribSelSpec()
{ return as; }

void visSurvey::PlaneDataDisplay::setAttribSelSpec( AttribSelSpec& as_ )
{ as = as_; }


CubeSampling& visSurvey::PlaneDataDisplay::getCubeSampling( bool manippos )
{
    cs.hrg.start = BinID( mNINT(manippos
				? trect->getRectangle().manipOrigo().x
				: trect->getRectangle().origo().x),
		  	  mNINT(manippos
				? trect->getRectangle().manipOrigo().y
				: trect->getRectangle().origo().y) );

    cs.hrg.stop = cs.hrg.start;
    cs.hrg.step = SI().step();

    cs.zrg.start = (manippos
	    ? trect->getRectangle().manipOrigo().z
	    : trect->getRectangle().origo().z);
    cs.zrg.stop = cs.zrg.start;

    if ( trect->getRectangle().orientation()==visBase::Rectangle::XY )
    {
	cs.hrg.stop.inl += mNINT(trect->getRectangle().width( 0 ));
	cs.hrg.stop.crl += mNINT(trect->getRectangle().width( 1 ));
    }
    else if ( trect->getRectangle().orientation()==visBase::Rectangle::XZ )
    {
	cs.hrg.stop.inl += mNINT(trect->getRectangle().width( 0 ));
	cs.zrg.stop += trect->getRectangle().width( 1 );
    }
    else
    {
	cs.hrg.stop.crl += mNINT(trect->getRectangle().width(1));
	cs.zrg.stop += trect->getRectangle().width(0);
    }

    return cs;
}


void visSurvey::PlaneDataDisplay::setCubeSampling( const CubeSampling& cs )
{
    Geometry::Pos width( cs.hrg.stop.inl - cs.hrg.start.inl,
			 cs.hrg.stop.crl - cs.hrg.start.crl, 
			 cs.zrg.stop - cs.zrg.start );
    setWidth( width );
    Geometry::Pos origo(cs.hrg.start.inl,cs.hrg.start.crl,cs.zrg.start);
    setOrigo( origo );
}


bool visSurvey::PlaneDataDisplay::putNewData( AttribSlice* attrslice )
{
    if ( !attrslice )
    {
	trect->getRectangle().resetManip();
	return false;
    }

    if ( trect->getRectangle().manipOrigo() != trect->getRectangle().origo() )
	trect->getRectangle().moveObjectToManipRect();
    
    trect->setData( *attrslice );
    trect->useTexture( true );
    
    return true;
}


void visSurvey::PlaneDataDisplay::turnOn(bool n) { trect->turnOn(n); }


bool visSurvey::PlaneDataDisplay::isOn() const { return trect->isOn(); }


float visSurvey::PlaneDataDisplay::getValue( const Geometry::Pos& pos ) const
{ return trect->getValue( pos ); }


void visSurvey::PlaneDataDisplay::setMaterial( visBase::Material* nm)
{ trect->setMaterial(nm); }


const visBase::Material* visSurvey::PlaneDataDisplay::getMaterial() const
{ return trect->getMaterial(); }


visBase::Material* visSurvey::PlaneDataDisplay::getMaterial()
{ return trect->getMaterial(); }


SoNode* visSurvey::PlaneDataDisplay::getData() { return trect->getData(); }


void visSurvey::PlaneDataDisplay::select()
{
    if ( selected_ ) return;
    selected_ = true;
}


void visSurvey::PlaneDataDisplay::deSelect()
{
    if ( !selected_ ) return;
    selected_ = false;

    if ( trect->getRectangle().isManipRectOnObject() ) return;

    if ( updateAtNewPos() )
	trect->getRectangle().moveObjectToManipRect();
    else
	trect->getRectangle().resetManip();
}
