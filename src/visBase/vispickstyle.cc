/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vispickstyle.cc,v 1.1 2004-01-05 09:43:23 kristofer Exp $";

#include "vispickstyle.h"
#include "iopar.h"

#include "Inventor/nodes/SoPickStyle.h"

mCreateFactoryEntry( visBase::PickStyle );

const char* visBase::PickStyle::stylestr = "Style";

visBase::PickStyle::PickStyle()
    : pickstyle( new SoPickStyle )
{
    pickstyle->ref();
}


visBase::PickStyle::~PickStyle()
{
    pickstyle->unref();
}


void visBase::PickStyle::setStyle( visBase::PickStyle::Style style )
{
    if ( style==Shape )
	pickstyle->style = SoPickStyle::SHAPE;
    else if ( style==BoundingBox )
	pickstyle->style = SoPickStyle::BOUNDING_BOX;
    else if ( style==Unpickable )
	pickstyle->style = SoPickStyle::UNPICKABLE;
}


visBase::PickStyle::Style visBase::PickStyle::getStyle() const
{
    if ( pickstyle->style.getValue()==SoPickStyle::SHAPE ) 
	return Shape;

    if ( pickstyle->style.getValue()==SoPickStyle::BOUNDING_BOX ) 
	return BoundingBox;

    return Unpickable;

}


SoNode* visBase::PickStyle::getInventorNode() { return pickstyle; }


int visBase::PickStyle::usePar( const IOPar& iopar )
{
    int res = DataObject::usePar( iopar );
    if ( res!=1 ) return res;

    int style;
    if ( !iopar.get(stylestr,style) )
	return -1; 

    setStyle( (Style) style );
    return 1;
}


void visBase::PickStyle::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( iopar, saveids );
    iopar.set( stylestr, (int) getStyle() );
}
