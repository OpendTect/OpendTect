/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visplanedatadisplay.cc,v 1.4 2002-04-22 12:34:29 kristofer Exp $";

#include "visplanedatadisplay.h"
#include "vissurvscene.h"
#include "geompos.h"
#include "cubesampling.h"
#include "attribsel.h"
#include "attribslice.h"
#include "vistexturerect.h"
#include "vissurvman.h"
#include "arrayndimpl.h"
#include "position.h"
#include "survinfo.h"
#include "visselman.h"
#include "visdataman.h"
#include "visrectangle.h"

#include "sorting.h"

visSurvey::PlaneDataDisplay::PlaneDataDisplay(
			visSurvey::PlaneDataDisplay::Type type_,
			visSurvey::Scene& scene_,
			const CallBack appcb )
    : VisualObject( true )
    , trect( visBase::TextureRect::create(true) )
    , type( type_ )
    , selected_( false )
    , cs(*new CubeSampling)
    , as(*new AttribSelSpec)
    , newdatacb(appcb)
    , scene( scene_ )
{
    trect->ref();
    selection()->notify( mCB(this,PlaneDataDisplay,select));
    deSelection()->notify( mCB(this,PlaneDataDisplay,deSelect));

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

    trect->getRectangle().setSnapping( true );
    trect->useTexture( false );
    resetDraggerSizes( scene.apparentVel() );

    scene.appvelchange.notify(
	    mCB( this, visSurvey::PlaneDataDisplay, updateDraggerCB ));
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
    trect->selection()->remove( mCB(this,PlaneDataDisplay,select));
    trect->deSelection()->remove( mCB(this,PlaneDataDisplay,deSelect));

    trect->unRef();
    scene.appvelchange.remove(
	    mCB( this, visSurvey::PlaneDataDisplay, updateDraggerCB ));
}


bool visSurvey::PlaneDataDisplay::getNewTextureData()
{
    succeeded_ = false;
    newdatacb.doCall( this );
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


bool visSurvey::PlaneDataDisplay::putNewData( AttribSlice* attrslice )
{
    if ( !attrslice )
    {
	trect->getRectangle().resetManip();
	return false;
    }
    
    trect->setData( *attrslice );
    trect->useTexture( true );
    
    return true;
}


void visSurvey::PlaneDataDisplay::turnOn(bool n) { trect->turnOn(n); }


bool visSurvey::PlaneDataDisplay::isOn() const { return trect->isOn(); }


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

    if ( getNewTextureData() )
	trect->getRectangle().moveObjectToManipRect();
    else
	trect->getRectangle().resetManip();
}


void visSurvey::PlaneDataDisplay::updateDraggerCB( CallBacker* cb )
{
    resetDraggerSizes( scene.apparentVel() );
}
