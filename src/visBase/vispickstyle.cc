/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: vispickstyle.cc,v 1.3 2005-02-07 12:45:40 nanne Exp $
________________________________________________________________________

-*/

#include "vispickstyle.h"
#include "iopar.h"

#include <Inventor/nodes/SoPickStyle.h>

mCreateFactoryEntry( visBase::PickStyle );

namespace visBase
{

const char* PickStyle::stylestr = "Style";

PickStyle::PickStyle()
    : pickstyle( new SoPickStyle )
{
    pickstyle->ref();
}


PickStyle::~PickStyle()
{
    pickstyle->unref();
}


void PickStyle::setStyle( PickStyle::Style style )
{
    if ( style==Shape )
	pickstyle->style = SoPickStyle::SHAPE;
    else if ( style==BoundingBox )
	pickstyle->style = SoPickStyle::BOUNDING_BOX;
    else if ( style==Unpickable )
	pickstyle->style = SoPickStyle::UNPICKABLE;
}


PickStyle::Style PickStyle::getStyle() const
{
    if ( pickstyle->style.getValue()==SoPickStyle::SHAPE ) 
	return Shape;

    if ( pickstyle->style.getValue()==SoPickStyle::BOUNDING_BOX ) 
	return BoundingBox;

    return Unpickable;

}


SoNode* PickStyle::getInventorNode() { return pickstyle; }


int PickStyle::usePar( const IOPar& iopar )
{
    int res = DataObject::usePar( iopar );
    if ( res!=1 ) return res;

    int style;
    if ( !iopar.get(stylestr,style) )
	return -1; 

    setStyle( (Style) style );
    return 1;
}


void PickStyle::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( iopar, saveids );
    iopar.set( stylestr, (int) getStyle() );
}

}; // namespace visBase
