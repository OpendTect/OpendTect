/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : June 2006
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "vispolygonoffset.h"

#include "Inventor/nodes/SoPolygonOffset.h"


namespace visBase
{
mCreateFactoryEntry( PolygonOffset );


PolygonOffset::PolygonOffset()
    : offset_( new SoPolygonOffset )
{
    offset_->ref();
}


PolygonOffset::~PolygonOffset()
{
    offset_->unref();
}


void PolygonOffset::setStyle( Style st )
{
    if ( st==Filled )
	offset_->styles.setValue( SoPolygonOffset::FILLED );
    else if ( st==Lines )
	offset_->styles.setValue( SoPolygonOffset::LINES );
    else if ( st==Points )
	offset_->styles.setValue( SoPolygonOffset::POINTS );
}


PolygonOffset::Style PolygonOffset::getStyle() const
{
    if ( offset_->styles.getValue()==SoPolygonOffset::FILLED )
	return Filled;

    if ( offset_->styles.getValue()==SoPolygonOffset::LINES )
	return Lines;

    return Points;
}


void PolygonOffset::setFactor( float f )
{
    offset_->factor.setValue( f );
}


float PolygonOffset::getFactor() const
{
    return offset_->factor.getValue();
}


void PolygonOffset::setUnits( float f )
{
    offset_->units.setValue( f );
}


float PolygonOffset::getUnits() const
{
    return offset_->units.getValue();
}


SoNode* PolygonOffset::gtInvntrNode()
{ return offset_; }


}; //namespace
