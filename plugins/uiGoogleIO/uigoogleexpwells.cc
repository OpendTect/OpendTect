/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2007
-*/


#include "uigoogleexpwells.h"
#include "googlexmlwriter.h"
#include "uifileinput.h"
#include "uiwellsel.h"
#include "uimsg.h"
#include "oddirs.h"
#include "survinfo.h"
#include "wellmanager.h"
#include "latlong.h"
#include "od_ostream.h"
#include "od_helpids.h"


uiGoogleExportWells::uiGoogleExportWells( uiParent* p )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrExport( tr("Wells to KML")),
				 tr("Specify wells to output"),
				 mODHelpKey(mGoogleExportWellsHelpID)))
{
    selfld_ = new uiMultiWellSel( this, false );

    mImplFileNameFld("wells");
    fnmfld_->attach( alignedBelow, selfld_ );
}


bool uiGoogleExportWells::acceptOK()
{
    mCreateWriter( "Wells", SI().name() );

    wrr.writeIconStyles( "wellpin", 20 );

    DBKeySet wellids;
    selfld_->getSelected( wellids );
    if ( wellids.isEmpty() )
    {
	uiMSG().error(uiStrings::phrPlsSelectAtLeastOne(uiStrings::sWell()));
	return false;
    }

    for ( int idx=0; idx<wellids.size(); idx++ )
    {
	const DBKey wellid = wellids[idx];
	const Coord coord = Well::MGR().getMapLocation( wellid );
	if ( coord.isUdf() )
	    continue;

	wrr.writePlaceMark( "wellpin", coord, Well::MGR().nameOf(wellid) );
	if ( !wrr.isOK() )
	    { uiMSG().error(wrr.errMsg()); return false; }
    }

    wrr.close();
    return true;
}
