/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visscalebardisplay.h"

#include "color.h"
#include "visdatagroup.h"
#include "visdataman.h"
#include "visevent.h"
#include "vismaterial.h"
#include "visscalebar.h"


mCreateFactoryEntry( visSurvey::ScaleBarDisplay );

static const char* sKeyOnInlCrl()	{ return "On Inl/Crl"; }
static const char* sKeyOrientation()	{ return "Orientation"; }
static const char* sKeyLineWidth()	{ return "Line width"; }
static const char* sKeyLength()		{ return "Length"; }

namespace visSurvey
{
// ScaleBarDisplay
ScaleBarDisplay::ScaleBarDisplay()
{
    ref();
    group_ = visBase::DataObjectGroup::create();
    addChild( group_->osgNode() );
    unRefNoDelete();
}


ScaleBarDisplay::~ScaleBarDisplay()
{
}



//
//void ScaleBarDisplay::setScene( Scene* ns )
//{
// //   /*if ( scene_ )
//	//scene_->zstretchchange.remove( mCB(this,ScaleBarDisplay,zScaleCB) );
// // */  SurveyObject::setScene( ns );
//
// //  /* if ( scene_ )
//	//scene_->zstretchchange.notify( mCB(this,ScaleBarDisplay,zScaleCB) );*/
//}


void ScaleBarDisplay::setDisplayTransformation( const mVisTrans* tf )
{
    displaytransform_ = tf;
}


const mVisTrans* ScaleBarDisplay::getDisplayTransformation() const
{
    return displaytransform_.ptr();
}

#define mToGroup( fn, val ) \
    for ( int idx=0; idx<group_->size(); idx++ ) \
    { \
	mDynamicCastGet(visBase::ScaleBar*,sb,group_->getObject(idx)); \
	if ( sb ) sb->fn( val ); \
    }

void ScaleBarDisplay::setLength( double l )
{
    length_ = l;
    mToGroup( setLength, l )
}

double ScaleBarDisplay::getLength() const
{ return length_; }


void ScaleBarDisplay::setLineWidth( int width )
{
    linewidth_ = width;
    mToGroup( setLineWidth, width )
}


int ScaleBarDisplay::getLineWidth() const
{ return linewidth_; }


void ScaleBarDisplay::setOrientation( int ortn )
{
    orientation_ = ortn;
    mToGroup( setOrientation, ortn )
}


int ScaleBarDisplay::getOrientation() const
{
    return orientation_;
}


void ScaleBarDisplay::setOnInlCrl( bool yn )
{
    oninlcrl_ = yn;
    mToGroup( setOnInlCrl, yn );
}


void ScaleBarDisplay::setColors( OD::Color color )
{
    mToGroup( setColor, color );
}


bool ScaleBarDisplay::isOnInlCrl() const
{
    return oninlcrl_;
}


void ScaleBarDisplay::zScaleCB( CallBacker* )
{
    fullRedraw();
}


void ScaleBarDisplay::dispChg( CallBacker* cb )
{
    LocationDisplay::dispChg( cb );
    setColors( getMaterial()->getColor() );
}


RefMan<visBase::VisualObject> ScaleBarDisplay::createLocation() const
{
    RefMan<visBase::ScaleBar> sb = visBase::ScaleBar::create();
    sb->setOnInlCrl( oninlcrl_ );
    sb->setLineWidth( linewidth_ );
    sb->setLength( length_ );
    sb->setOrientation( orientation_ );
    sb->setColor( getMaterial()->getColor() );
    sb->setMaterial( nullptr );
    sb->setDisplayTransformation( displaytransform_.ptr() );
    return sb;
}


void ScaleBarDisplay::setPosition( int idx, const Pick::Location& loc )
{
    setPosition( idx, loc, true );
}


void ScaleBarDisplay::setPosition( int idx, const Pick::Location& loc, bool add)
{
    if ( loc.dir().isNull() )
	return;

    const Coord3 normal = spherical2Cartesian( loc.dir(), true );
    const bool pickedonz = mIsEqual(normal.z_,1,mDefEps);
    if ( idx==0 )
	setOnInlCrl( !pickedonz );
    else if ( pickedonz == oninlcrl_ )
	return;

    mDynamicCastGet(visBase::ScaleBar*,sb,group_->getObject(idx));
    if ( sb )
	sb->setPick( loc );
    else
    {
	RefMan<visBase::VisualObject> sbloc = createLocation();
	auto* scb = sCast( visBase::ScaleBar*, sbloc.ptr() );
	scb->setPick( loc );
	group_->addObject( scb );
    }
}


void ScaleBarDisplay::removePosition( int idx )
{
    if ( idx >= group_->size() )
	return;

    group_->removeObject( idx );
}


int ScaleBarDisplay::clickedMarkerIndex(const visBase::EventInfo& evi)const
{
    for ( int idx=0; idx<group_->size(); idx++ )
    {
	mDynamicCastGet(const visBase::ScaleBar*,sb,group_->getObject(idx));
	if ( sb && evi.pickedobjids.isPresent(sb->id()) )
	    return idx;
    }

    return -1;
}


void ScaleBarDisplay::fromPar( const IOPar& par )
{
    bool oninlcrl = true;
    par.getYN( sKeyOnInlCrl(), oninlcrl );
    setOnInlCrl( oninlcrl );

    int orientation = 0;
    par.get( sKeyOrientation(), orientation );
    setOrientation( orientation );

    int linewidth = 2;
    par.get( sKeyLineWidth(), linewidth );
    setLineWidth( linewidth );

    double length = 1000;
    par.get( sKeyLength(), length );
    setLength( length );
}


void ScaleBarDisplay::toPar( IOPar& par ) const
{
    par.setYN( sKeyOnInlCrl(), isOnInlCrl() );
    par.set( sKeyOrientation(), getOrientation() );
    par.set( sKeyLineWidth(), getLineWidth() );
    par.set( sKeyLength(), getLength() );
}

} // namespace visSurvey
