/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          August 2002
 RCS:           $Id: visvolumedisplay.cc,v 1.3 2002-09-30 15:39:49 bert Exp $
________________________________________________________________________

-*/


#include "visvolumedisplay.h"
#include "viscubeview.h"
#include "visboxdragger.h"
#include "vistexturerect.h"
#include "visrectangle.h"
#include "geompos.h"
#include "cubesampling.h"
#include "attribsel.h"
#include "attribslice.h"
#include "arrayndimpl.h"
#include "survinfo.h"
#include "visselman.h"
#include "visdataman.h"
#include "sorting.h"
#include "iopar.h"
#include "colortab.h"
#include "viscolortab.h"

mCreateFactoryEntry( visSurvey::VolumeDisplay );

const char* visSurvey::VolumeDisplay::volumestr = "Volume box";

visSurvey::VolumeDisplay::VolumeDisplay()
    : VisualObject( true )
    , cube( visBase::CubeView::create() )
    , selected_( false )
    , as(*new AttribSelSpec)
    , prevcs(*new CubeSampling)
{
    cube->ref();
    selection()->notify( mCB(this,VolumeDisplay,select));
    deSelection()->notify( mCB(this,VolumeDisplay,deSelect));

    cube->dragger()->motion.notify( mCB(this,VolumeDisplay,manipInMotion) );
    cube->dragger()->finished.notify( mCB(this,VolumeDisplay,manipFinished) );

    setCenter( Geometry::Pos((SI().range().stop.inl + SI().range().start.inl)/2,
			     (SI().range().stop.crl + SI().range().start.crl)/2,
			     (SI().zRange().stop + SI().zRange().start)/2 ) );

    setWidth( Geometry::Pos( (SI().range().stop.inl - SI().range().start.inl)/4,
			     (SI().range().stop.crl - SI().range().start.crl)/4,
			     (SI().zRange().stop - SI().zRange().start)/4 ) );

    prevcs = getCubeSampling();
    cube->initPlanes( prevcs );
    cube->showBox( true );
}


visSurvey::VolumeDisplay::~VolumeDisplay()
{
    selection()->remove( mCB(this,VolumeDisplay,select));
    deSelection()->remove( mCB(this,VolumeDisplay,deSelect));

    cube->unRef();

    delete &as;
    delete &prevcs;
}


void visSurvey::VolumeDisplay::setCenter( const Geometry::Pos& pos )
{ cube->setCenter( pos ); }


Geometry::Pos visSurvey::VolumeDisplay::center() const
{ return cube->center(); }


void visSurvey::VolumeDisplay::setWidth( const Geometry::Pos& pos )
{ cube->setWidth( pos ); }


Geometry::Pos visSurvey::VolumeDisplay::width() const
{ return cube->width(); }


void visSurvey::VolumeDisplay::showBox( bool yn )
{ cube->showBox( yn ); }


void visSurvey::VolumeDisplay::resetManip()
{
}


void visSurvey::VolumeDisplay::getPlaneIds( int& inlid, int& crlid, int& tslid )
{
    inlid = cube->inlPlane()->id();
    crlid = cube->crlPlane()->id();
    tslid = cube->tslPlane()->id();
}


float visSurvey::VolumeDisplay::getPlanePos( int dim )
{
    Geometry::Pos pos;
    switch( dim )
    {
	case 0: {
	    pos = cube->inlPlane()->getRectangle().manipOrigo();
	    return pos.x;
	    } break;
	case 1: {
	    pos = cube->crlPlane()->getRectangle().manipOrigo();
	    return pos.y;
	    } break;
	case 2: {
	    pos = cube->tslPlane()->getRectangle().manipOrigo();
	    return pos.z;
	    } break;
	default:
	    return 0;
    }
}


bool visSurvey::VolumeDisplay::updateAtNewPos()
{
    succeeded_ = false;
    return succeeded_;
}


AttribSelSpec& visSurvey::VolumeDisplay::getAttribSelSpec()
{ return as; }


void visSurvey::VolumeDisplay::setAttribSelSpec( AttribSelSpec& as_ )
{ as = as_; }


CubeSampling& visSurvey::VolumeDisplay::getCubeSampling()
{
    Geometry::Pos center_ = center();
    Geometry::Pos width_ = width();

    CubeSampling* cs = new CubeSampling;
    cs->hrg.start = BinID( mNINT( center_.x - width_.x / 2 ),
		  	  mNINT( center_.y - width_.y / 2 ) );

    cs->hrg.stop = BinID( mNINT( center_.x + width_.x / 2 ),
		  	 mNINT( center_.y + width_.y / 2 ) );

    cs->hrg.step = BinID( SI().inlStep(), SI().crlStep() );

    cs->zrg.start = center_.z - width_.z / 2;
    cs->zrg.stop = center_.z + width_.z / 2;

    return *cs;
}


void visSurvey::VolumeDisplay::setCubeSampling( const CubeSampling& cs )
{
    Geometry::Pos width( cs.hrg.stop.inl - cs.hrg.start.inl,
			 cs.hrg.stop.crl - cs.hrg.start.crl, 
			 cs.zrg.stop - cs.zrg.start );
    setWidth( width );
    Geometry::Pos center( (cs.hrg.stop.inl + cs.hrg.start.inl) / 2,
			  (cs.hrg.stop.crl + cs.hrg.start.crl) / 2,
			  (cs.zrg.stop + cs.zrg.start) / 2 );
    setCenter( center );
    cube->initPlanes( cs );
}


bool visSurvey::VolumeDisplay::putNewData( AttribSliceSet* sliceset )
{
    prevcs = sliceset->sampling;
    return true;
}


AttribSliceSet* visSurvey::VolumeDisplay::getPrevData()
{
    return 0;
}


void visSurvey::VolumeDisplay::turnOn(bool n) 
{ cube->turnOn(n); }


bool visSurvey::VolumeDisplay::isOn() const 
{ return cube->isOn(); }


void visSurvey::VolumeDisplay::select()
{
    if ( selected_ ) return;
    selected_ = true;
}


void visSurvey::VolumeDisplay::deSelect()
{
    if ( !selected_ ) return;
    selected_ = false;
}


void visSurvey::VolumeDisplay::manipFinished( CallBacker* )
{
    CubeSampling cs = getCubeSampling();
    BinIDRange br;
    br.start = cs.hrg.start;
    br.stop = cs.hrg.stop;
    SI().checkRange( br );
    cs.hrg.start = br.start;
    cs.hrg.stop = br.stop;

    Interval<double> intv( cs.zrg.start, cs.zrg.stop );
    SI().checkZRange( intv );
    cs.zrg.start = (float)intv.start;
    cs.zrg.stop = (float)intv.stop;
    
    setCubeSampling( cs );
    cube->initPlanes( cs );
}


void visSurvey::VolumeDisplay::manipInMotion( CallBacker* )
{
    CubeSampling cs = getCubeSampling();
    cube->initPlanes( cs );    
}


void visSurvey::VolumeDisplay::setColorTable( const ColorTable& ctab )
{
    cube->inlPlane()->getColorTab().colorSeq().colors() = ctab;
    cube->inlPlane()->getColorTab().colorSeq().colorsChanged();
}


const ColorTable& visSurvey::VolumeDisplay::getColorTable() const
{ return cube->inlPlane()->getColorTab().colorSeq().colors(); }


void visSurvey::VolumeDisplay::setClipRate( float rate )
{
    cube->inlPlane()->setClipRate( rate );
    cube->crlPlane()->setClipRate( rate );
    cube->tslPlane()->setClipRate( rate );
}


float visSurvey::VolumeDisplay::clipRate() const
{ return cube->inlPlane()->clipRate(); }


void visSurvey::VolumeDisplay::setAutoscale( bool yn )
{
    cube->inlPlane()->setAutoscale( yn );
    cube->crlPlane()->setAutoscale( yn );
    cube->tslPlane()->setAutoscale( yn );
}


bool visSurvey::VolumeDisplay::autoScale() const
{ return cube->inlPlane()->autoScale(); }


void visSurvey::VolumeDisplay::setMaterial( visBase::Material* nm)
{ cube->inlPlane()->setMaterial(nm); }


const visBase::Material* visSurvey::VolumeDisplay::getMaterial() const
{ return cube->inlPlane()->getMaterial(); }


visBase::Material* visSurvey::VolumeDisplay::getMaterial()
{ return cube->inlPlane()->getMaterial(); }


SoNode* visSurvey::VolumeDisplay::getData() 
{ return cube->getData(); }


void visSurvey::VolumeDisplay::fillPar( IOPar& par, TypeSet<int>& saveids) const
{
    visBase::VisualObject::fillPar( par, saveids );
    int volid = cube->id();
    par.set( volumestr, volid );

    if ( saveids.indexOf( volid )==-1 ) saveids += volid;

    as.fillPar(par);
}


int visSurvey::VolumeDisplay::usePar( const IOPar& par )
{
    int res =  visBase::VisualObject::usePar( par );
    if ( res!=1 ) return res;

    int volid;
    if ( !par.get( volumestr, volid ))
	return -1;

    visBase::DataObject* dataobj = visBase::DM().getObj( volid );
    if ( !dataobj ) return 0;

    mDynamicCastGet(visBase::CubeView*,cv,dataobj);
    if ( !cv ) return -1;

    cube->unRef();
    cube = cv;
    cube->ref();

    if ( !as.usePar( par )) return -1;

    return 1;
}
    


