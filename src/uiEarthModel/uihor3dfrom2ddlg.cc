/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          January 2007
 RCS:           $Id: uihor3dfrom2ddlg.cc,v 1.2 2007-01-24 17:19:43 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uihor3dfrom2ddlg.h"

#include "emhorizon2d.h"


uiHor3DFrom2DDlg::uiHor3DFrom2DDlg( uiParent* p, const EM::Horizon2D& h2d )
    : uiDialog( p, Setup("Derive 3D horizon") )
    , hor2d_( h2d )
{
}


bool uiHor3DFrom2DDlg::acceptOK( CallBacker* )
{
    return true;
}
