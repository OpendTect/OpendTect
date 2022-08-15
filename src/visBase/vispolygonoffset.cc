/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : June 2006
-*/


#include "vispolygonoffset.h"

#include <osg/PolygonOffset>


namespace visBase
{


PolygonOffset::PolygonOffset()
    : offset_( addAttribute( new osg::PolygonOffset ) )
{
    offset_->ref();
    mode_ = (unsigned int) ( On );
}


PolygonOffset::~PolygonOffset()
{
    offset_->unref();
}


void PolygonOffset::setFactor( float f )
{
    offset_->setFactor( f );
}


float PolygonOffset::getFactor() const
{
    return offset_->getFactor();
}


void PolygonOffset::setUnits( float f )
{
    offset_->setUnits( f );
}


float PolygonOffset::getUnits() const
{
    return offset_->getUnits();
}

void PolygonOffset::applyAttribute( osg::StateSet* ns,
    osg::StateAttribute* attr)
{
    osg::StateAttribute::GLMode mvalue = (osg::StateAttribute::GLMode)mode_;
    ns->setAttributeAndModes( attr, mvalue );
}


void PolygonOffset::setMode( unsigned int modevalue )
{
    mode_ =  modevalue;
}

} // namespace visBase
