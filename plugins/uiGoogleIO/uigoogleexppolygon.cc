/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

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
    uiSelLineStyle::Setup lssu; lssu.drawstyle( false ).transparency( true );
    lsfld_ = new uiSelLineStyle( this, ls, lssu );

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

    const float reqwdth = lsfld_->getWidth() * 0.1f;
    wrr.writePolyStyle( "polygon", lsfld_->getColor(), mNINT32(reqwdth) );
    wrr.writePoly( "polygon", ps_.name(), coords, hghtfld_->getfValue() );

    return true;
}
