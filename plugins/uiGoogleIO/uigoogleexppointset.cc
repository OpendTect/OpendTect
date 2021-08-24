/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Wayne Mogg
 * DATE     : Dec 2019
-*/


#include "uigoogleexppointset.h"
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
#include "od_helpids.h"

#include <iostream>


uiGoogleExportPointSet::uiGoogleExportPointSet( uiParent* p,
						const Pick::Set& ps )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrExport( tr("PointSet to KML")),
				 tr("Specify output parameters"),
				 mODHelpKey(mGoogleExportPolygonHelpID) ) )
    , ps_(ps)
{
    hghtfld_ = new uiGenInput( this, tr("Height"), FloatInpSpec(100) );

    mImplFileNameFld(ps.name());
    fnmfld_->attach( alignedBelow, hghtfld_ );
}



bool uiGoogleExportPointSet::acceptOK( CallBacker* )
{
    mCreateWriter( "PointSet", SI().name() );

    for ( int idx=0; idx<ps_.size(); idx++ )
    {
	const Coord coord = ps_[idx].pos_;
	wrr.writePlaceMark( 0, coord, BufferString::empty(),
			    hghtfld_->getFValue() );
    }

    return true;
}
