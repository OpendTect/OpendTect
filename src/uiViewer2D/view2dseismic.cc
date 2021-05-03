/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
________________________________________________________________________

-*/

#include "view2dseismic.h"


VW2DSeis::VW2DSeis()
    : Vw2DDataObject()
    , deselted_( this )
{}


void VW2DSeis::triggerDeSel()
{
    deselted_.trigger();
}
