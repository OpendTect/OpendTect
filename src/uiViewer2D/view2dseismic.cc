/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "view2dseismic.h"

namespace View2D
{

Seismic::Seismic()
    : DataObject()
    , deselected_( this )
{}


void Seismic::triggerDeSel()
{
    deselected_.trigger();
}

} // namespace View2D
