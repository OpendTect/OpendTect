/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2002
-*/

static const char* rcsID = "$Id: vispicksetdisplay.cc,v 1.94 2007-07-09 16:47:00 cvsbert Exp $";

#include "vispicksetdisplay.h"

#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "pickset.h"
#include "picksettr.h"
#include "separstr.h"
#include "visdrawstyle.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "vispolyline.h"


mCreateFactoryEntry( visSurvey::PickSetDisplay );

namespace visSurvey {

const char* PickSetDisplay::sKeyNrPicks()       { return "No Picks"; }
const char* PickSetDisplay::sKeyPickPrefix()    { return "Pick "; }


PickSetDisplay::PickSetDisplay()
    : polyline_(0)
    , needline_(false)
{}


PickSetDisplay::~PickSetDisplay()
{
    if ( polyline_ ) removeChild( polyline_->getInventorNode() );
}


visBase::VisualObject* PickSetDisplay::createLocation() const
{
    visBase::Marker* marker = visBase::Marker::create();
    marker->setType( (MarkerStyle3D::Type) set_->disp_.markertype_ );
    marker->setScreenSize( set_->disp_.pixsize_ );
    marker->setMaterial( 0 );
    return marker;
}


void PickSetDisplay::createLine()
{
    if ( !polyline_ ) 
    {
	polyline_ = visBase::PolyLine::create();
	addChild( polyline_->getInventorNode() );
	polyline_->setDisplayTransformation( transformation_ );
	polyline_->setMaterial( 0 );
    }

    int pixsize = set_->disp_.pixsize_;
    LineStyle ls;
    ls.width_ = pixsize;
    polyline_->setLineStyle( ls );
    while ( polyline_->size() > group_->size() )
	polyline_->removePoint( 0 );

    for ( int idx=0; idx<group_->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*,marker,group_->getObject(idx));
	polyline_->setPoint( idx, marker->centerPos() );
    }

    int nrnodes = polyline_->size();
    if ( nrnodes ) 
	polyline_->setPoint( nrnodes, polyline_->getPoint(0) );
} 


void PickSetDisplay::showLine( bool yn )
{
    if ( yn ) needline_ = true;
    if ( !needline_ ) return;
    if ( !polyline_ ) createLine();
    polyline_->turnOn( yn );
}


bool PickSetDisplay::lineShown()
{
    return polyline_ ? polyline_->isOn() : false;
}


void PickSetDisplay::setPosition( int idx, const Pick::Location& loc )
{
    mDynamicCastGet(visBase::Marker*,marker,group_->getObject(idx));
    marker->setCenterPos( loc.pos );
    marker->setDirection( loc.dir );
}


Coord3 PickSetDisplay::getPosition( int loc ) const 
{
    mDynamicCastGet( visBase::Marker*, marker, group_->getObject(loc) );
    return marker->centerPos();
}


::Sphere PickSetDisplay::getDirection( int loc ) const 
{
    mDynamicCastGet( visBase::Marker*, marker, group_->getObject(loc) );
    return marker->getDirection();
}


void PickSetDisplay::locChg( CallBacker* cb )
{
    LocationDisplay::locChg( cb );
    if ( needline_ ) createLine();
}


void PickSetDisplay::dispChg( CallBacker* cb )
{
    mDynamicCastGet(visBase::Marker*,firstmarker,group_->getObject(0));
    if ( firstmarker )
    {
	const int oldpixsz = (int)(firstmarker->getScreenSize() + .5);
	if ( oldpixsz != set_->disp_.pixsize_ )
	{
	    if ( needline_ && polyline_ )
	    {
		LineStyle ls; ls.width_ = set_->disp_.pixsize_;
		polyline_->setLineStyle( ls );
	    }

	    for ( int idx=0; idx<group_->size(); idx++ )
	    {
		mDynamicCastGet(visBase::Marker*,marker,
				group_->getObject(idx));
		if ( marker )
		    marker->setScreenSize( set_->disp_.pixsize_ );
	    }
	}

	if ( (int)firstmarker->getType() != set_->disp_.markertype_ )
	{
	    for ( int idx=0; idx<group_->size(); idx++ )
	    {
		mDynamicCastGet(visBase::Marker*,marker,
				group_->getObject(idx))
		if ( marker )
		{
		    marker->setType(
			    (MarkerStyle3D::Type)set_->disp_.markertype_ );
		}
	    }
	}
    }

    LocationDisplay::dispChg( cb );
}


int PickSetDisplay::isMarkerClick( const TypeSet<int>& path ) const
{
    for ( int idx=group_->size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet(visBase::Marker*,marker,group_->getObject(idx));
	if ( !marker )
	    continue;

	if ( path.indexOf(marker->id())!=-1 )
	    return idx;
    }

    return -1;
}


void PickSetDisplay::setDisplayTransformation( visBase::Transformation* newtr )
{
    LocationDisplay::setDisplayTransformation( newtr );
    if (polyline_ ) polyline_->setDisplayTransformation( newtr );
}


int PickSetDisplay::usePar( const IOPar& par )
{
    int nopicks = 0;
    if ( par.get(sKeyNrPicks(),nopicks) ) // old format
    {
	int res =  visBase::VisualObjectImpl::usePar( par );
	if ( res != 1 ) return res;

	Pick::Set* newps = new Pick::Set;
	for ( int idx=0; idx<nopicks; idx++ )
	{
	    BufferString str;
	    BufferString key = sKeyPickPrefix(); key += idx;
	    if ( !par.get(key,str) )
		return -1;

	    FileMultiString fms( str );
	    Coord3 pos( atof(fms[0]), atof(fms[1]), atof(fms[2]) );
	    Sphere dir;
	    if ( fms.size() > 3 )
		dir = Sphere( atof(fms[3]), atof(fms[4]), atof(fms[5]) );

	    *newps += Pick::Location( pos, dir );
	}

	int markertype = 0;
	int pixsize = 3;

	par.get( sKeyMarkerType(), markertype );
	par.get( sKeyMarkerSize(), pixsize );
	bool shwallpicks = true;
	par.getYN( sKeyShowAll(), shwallpicks );
	showAll( shwallpicks );

	newps->disp_.markertype_ = markertype;
	newps->disp_.pixsize_ = pixsize;
	newps->disp_.color_ = getMaterial()->getColor();

	BufferString psname;
	par.get( sKey::Name, psname );
	newps->setName( psname );
	setSet( newps );

	PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(PickSet);
	IOM().to( ctio->ctxt.getSelKey() );
	const IOObj* existioobj = IOM().getLocal( psname );
	if ( existioobj )
	    storedmid_ = existioobj->key();
	else
	{
	    ctio->setName( psname );
	    IOM().getEntry( *ctio );
	    storedmid_ = ctio->ioobj->key();
	}
    }
    else
    {
	int res =  visSurvey::LocationDisplay::usePar( par );
	if ( res != 1 ) return res;
    }

    return 1;
}

}; // namespace visSurvey
