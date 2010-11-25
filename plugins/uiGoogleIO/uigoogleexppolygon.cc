/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2007
-*/

static const char* rcsID = "$Id: uigoogleexppolygon.cc,v 1.6 2010-11-25 09:20:08 cvsnanne Exp $";

#include "uigoogleexppolygon.h"
#include "googlexmlwriter.h"
#include "uisellinest.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "oddirs.h"
#include "strmprov.h"
#include "survinfo.h"
#include "latlong.h"
#include "pickset.h"
#include "draw.h"
#include <iostream>


uiGoogleExportPolygon::uiGoogleExportPolygon( uiParent* p, const Pick::Set& ps )
    : uiDialog(p,uiDialog::Setup("Export Polygon to KML",
				 "Specify output parameters","105.1.0") )
    , ps_(ps)
{
    Color defcol( ps_.disp_.color_ ); defcol.setTransparency( 150 );
    const LineStyle ls( LineStyle::Solid, 20, defcol );
    lsfld_ = new uiSelLineStyle( this, ls, "Line style", false, true, true );
    lsfld_->enableTransparency( true );

    hghtfld_ = new uiGenInput( this, "Height", FloatInpSpec(100) );
    hghtfld_->attach( alignedBelow, lsfld_ );

    mImplFileNameFld(ps.name());
    fnmfld_->attach( alignedBelow, hghtfld_ );
}



bool uiGoogleExportPolygon::acceptOK( CallBacker* )
{
    mCreateWriter( "Polygon", SI().name() );

    TypeSet<Coord> coords;
    for ( int idx=0; idx<ps_.size(); idx++ )
	coords += ps_[idx].pos;
    coords += ps_[0].pos;

    const float reqwdth = lsfld_->getWidth() * 0.1;
    wrr.writePolyStyle( "polygon", lsfld_->getColor(), mNINT(reqwdth) );
    wrr.writePoly( "polygon", ps_.name(), coords, hghtfld_->getfValue() );

    return true;
}
