/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id: visvw2dseismic.cc,v 1.1 2010/06/24 08:41:01 cvsumesh Exp $
________________________________________________________________________

-*/

#include "visvw2dseismic.h"


VW2DSeis::VW2DSeis()
    : Vw2DDataObject()
    , deselted_( this )
{}


void VW2DSeis::triggerDeSel()
{
    deselted_.trigger();
}
