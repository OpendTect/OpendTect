/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          August 2002
 RCS:           $Id: visvolumedisplay.cc,v 1.9 2002-11-13 10:42:25 nanne Exp $
________________________________________________________________________

-*/


#include "visvolumedisplay.h"
#include "viscubeview.h"
#include "visboxdragger.h"
#include "vistexturerect.h"
#include "visrectangle.h"
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

const char* visSurvey::VolumeDisplay::volumestr = "Volume dragger";

visSurvey::VolumeDisplay::VolumeDisplay()
    : VisualObject(true)
    , cube(visBase::CubeView::create())
    , selected_(false)
    , as(*new AttribSelSpec)
    , prevcs(*new CubeSampling)
    , moved(this)
    , manipulated(false)
    , slicemoving(this)
{
    cube->ref();
    selection()->notify( mCB(this,VolumeDisplay,select));
    deSelection()->notify( mCB(this,VolumeDisplay,deSelect));

    cube->dragger()->motion.notify( mCB(this,VolumeDisplay,manipInMotion) );
    cube->dragger()->finished.notify( mCB(this,VolumeDisplay,manipFinished) );

    prevcs.hrg.start.inl = (5*SI().range().start.inl+3*SI().range().stop.inl)/8;
    prevcs.hrg.start.crl = (5*SI().range().start.crl+3*SI().range().stop.crl)/8;
    prevcs.hrg.stop.inl = (3*SI().range().start.inl+5*SI().range().stop.inl)/8;
    prevcs.hrg.stop.crl = (3*SI().range().start.crl+5*SI().range().stop.crl)/8;
    prevcs.zrg.start = ( 5*SI().zRange().start + 3*SI().zRange().stop ) / 8;
    prevcs.zrg.stop = ( 3*SI().zRange().start + 5*SI().zRange().stop ) / 8;
    SI().snap( prevcs.hrg.start, BinID(0,0) );
    SI().snap( prevcs.hrg.stop, BinID(0,0) );
    setCubeSampling( prevcs );

    cube->showBox( true );
    inlid = cube->addSlice( 0 );
    crlid = cube->addSlice( 1 );
    tslid = cube->addSlice( 2 );
}


visSurvey::VolumeDisplay::~VolumeDisplay()
{
    selection()->remove( mCB(this,VolumeDisplay,select));
    deSelection()->remove( mCB(this,VolumeDisplay,deSelect));

    cube->unRef();

    delete &as;
    delete &prevcs;
}


void visSurvey::VolumeDisplay::setCenter( const Coord3& pos )
{ cube->setCenter( pos ); }


Coord3 visSurvey::VolumeDisplay::center() const
{ return cube->center(); }


void visSurvey::VolumeDisplay::setWidth( const Coord3& pos )
{ cube->setWidth( pos ); }


Coord3 visSurvey::VolumeDisplay::width() const
{ return cube->width(); }


void visSurvey::VolumeDisplay::showBox( bool yn )
{ cube->showBox( yn ); }


void visSurvey::VolumeDisplay::resetManip()
{
}


void visSurvey::VolumeDisplay::getPlaneIds( int& id0, int& id1, int& id2 )
{
    id0 = inlid;
    id1 = crlid;
    id2 = tslid;
}


float visSurvey::VolumeDisplay::getPlanePos( int id )
{
    return cube->slicePosition( id );
}


bool visSurvey::VolumeDisplay::updateAtNewPos()
{
    succeeded_ = false;
    moved.trigger();
    manipulated = false;
    return succeeded_;
}


AttribSelSpec& visSurvey::VolumeDisplay::getAttribSelSpec()
{ return as; }


void visSurvey::VolumeDisplay::setAttribSelSpec( AttribSelSpec& as_ )
{ as = as_; }


CubeSampling& visSurvey::VolumeDisplay::getCubeSampling()
{
    Coord3 center_ = cube->draggerCenter();
    Coord3 width_ = cube->draggerWidth();

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
    Coord3 width( cs.hrg.stop.inl - cs.hrg.start.inl,
			 cs.hrg.stop.crl - cs.hrg.start.crl, 
			 cs.zrg.stop - cs.zrg.start );
    setWidth( width );
    Coord3 center( (cs.hrg.stop.inl + cs.hrg.start.inl) / 2,
			  (cs.hrg.stop.crl + cs.hrg.start.crl) / 2,
			  (cs.zrg.stop + cs.zrg.start) / 2 );
    setCenter( center );
}


bool visSurvey::VolumeDisplay::putNewData( AttribSliceSet* sliceset )
{
    prevcs = sliceset->sampling;
    ObjectSet< const Array2D<float> > newset;
    for ( int idx=0; idx<sliceset->size(); idx++ )
	newset += (*sliceset)[idx];

    cube->setData( newset, (int)sliceset->direction );

    return true;
}


AttribSliceSet* visSurvey::VolumeDisplay::getPrevData()
{
    ObjectSet< const Array2D<float> >& data = cube->get3DData();
    AttribSliceSet* sliceset = new AttribSliceSet;
    for ( int idx=0; idx<data.size(); idx++ )
	(*sliceset) += (AttribSlice*)data[idx];

    sliceset->direction = (AttribSlice::Dir)cube->dataDirection();
    sliceset->sampling = prevcs;
    return sliceset->size() ? sliceset : 0;
}


void visSurvey::VolumeDisplay::turnOn( bool yn ) 
{ cube->turnOn( yn ); }


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

    
    if ( manipulated )
	updateAtNewPos();
}


void visSurvey::VolumeDisplay::manipFinished( CallBacker* )
{
    manipulated = true;

    CubeSampling cs = getCubeSampling();
    BinIDRange br;
    br.start = cs.hrg.start;
    br.stop = cs.hrg.stop;
    SI().checkRange( br );
    cs.hrg.start = br.start;
    cs.hrg.stop = br.stop;
    SI().snap( cs.hrg.start, BinID(0,0) );
    SI().snap( cs.hrg.stop, BinID(0,0) );

    Interval<double> intv( cs.zrg.start, cs.zrg.stop );
    SI().checkZRange( intv );
    cs.zrg.start = (float)intv.start;
    cs.zrg.stop = (float)intv.stop;
    
    setCubeSampling( cs );
    cube->resetDragger();
}


void visSurvey::VolumeDisplay::manipInMotion( CallBacker* )
{
}


void visSurvey::VolumeDisplay::setColorTable( const ColorTable& ctab )
{
    cube->getColorTab().colorSeq().colors() = ctab;
    cube->getColorTab().colorSeq().colorsChanged();
}


const ColorTable& visSurvey::VolumeDisplay::getColorTable() const
{ return cube->getColorTab().colorSeq().colors(); }


void visSurvey::VolumeDisplay::setClipRate( float rate )
{ cube->setClipRate( rate ); }


float visSurvey::VolumeDisplay::clipRate() const
{ return cube->clipRate(); }


void visSurvey::VolumeDisplay::setAutoscale( bool yn )
{ cube->setAutoscale( yn ); }


bool visSurvey::VolumeDisplay::autoScale() const
{ return cube->autoScale(); }


void visSurvey::VolumeDisplay::setDataRange( const Interval<float>& intv )
{ cube->getColorTab().scaleTo( intv ); }


Interval<float> visSurvey::VolumeDisplay::getDataRange() const
{ return cube->getColorTab().getInterval(); }


void visSurvey::VolumeDisplay::setMaterial( visBase::Material* nm)
{ cube->setMaterial(nm); }


const visBase::Material* visSurvey::VolumeDisplay::getMaterial() const
{ return cube->getMaterial(); }


visBase::Material* visSurvey::VolumeDisplay::getMaterial()
{ return cube->getMaterial(); }


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
