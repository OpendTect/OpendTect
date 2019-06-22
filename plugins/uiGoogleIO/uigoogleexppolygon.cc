/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2007
-*/


#include "uigoogleexppolygon.h"
#include "googlexmlwriter.h"
#include "uisellinest.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uimsg.h"
#include "oddirs.h"
#include "survinfo.h"
#include "latlong.h"
#include "pickset.h"
#include "draw.h"
#include "od_helpids.h"
#include "uiseparator.h"

#include <iostream>


uiGISExportPolygon::uiGISExportPolygon( uiParent* p, const Pick::Set& ps )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrExport( tr("Polygon to GIS")),
				 tr("Specify output parameters"),
				 mODHelpKey(mGoogleExportPolygonHelpID) ) )
    , ps_(ps)
{
    Color defcol( ps_.dispColor() ); defcol.setTransparency( 150 );
    const OD::LineStyle ls( OD::LineStyle::Solid, 20, defcol );
    uiSelLineStyle::Setup lssu; lssu.drawstyle( false ).transparency( true );
    lsfld_ = new uiSelLineStyle( this, ls, lssu );

    hghtfld_ = new uiGenInput( this, uiStrings::sHeight(), FloatInpSpec(100) );
    hghtfld_->attach( alignedBelow, lsfld_ );

    uiSeparator* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, hghtfld_ );
    expfld_ = new uiGISExpStdFld( this, ps.name() );
    expfld_->attach( stretchedBelow, sep );
    expfld_->attach( leftAlignedBelow, hghtfld_ );
}

bool uiGISExportPolygon::acceptOK()
{
    if ( ps_.isEmpty() )
	{ uiMSG().error( tr("Polygon is empty") ); return false; }

    TypeSet<Coord> coords;
    Pick::SetIter psiter( ps_ );
    while ( psiter.next() )
	coords += psiter.getPos();
    if ( ps_.isPolygon() )
	coords += ps_.first().pos().getXY();
    psiter.retire();

    GISWriter* wrr = expfld_->createWriter();
    if ( !wrr )
	return false; // Put some error message here

    GISWriter::Property props;
    props.color_ = lsfld_->getColor();
    props.width_ = mNINT32( lsfld_->getWidth() * 0.1f );
    props.iconnm_ = "Polygon";
    wrr->setProperties( props );
    wrr->writePolygon( coords, ps_.getName() );
    wrr->close();
    bool ret = uiMSG().askGoOn(
			tr("Successfully created %1 for selected Polygon"
	     " Do you want to create more?").arg(wrr->factoryDisplayName()) );
    return !ret;
}
