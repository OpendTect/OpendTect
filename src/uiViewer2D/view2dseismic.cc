/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
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
