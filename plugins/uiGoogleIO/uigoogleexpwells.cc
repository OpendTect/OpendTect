/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2007
-*/


#include "uigoogleexpwells.h"
#include "googlexmlwriter.h"
#include "uiwellsel.h"
#include "uimsg.h"
#include "oddirs.h"
#include "survinfo.h"
#include "wellmanager.h"
#include "latlong.h"
#include "od_ostream.h"
#include "od_helpids.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uiseparator.h"

uiGISExportWells::uiGISExportWells( uiParent* p )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrExport( tr("Wells to GIS")),
				 tr("Specify wells to output"),
				 mODHelpKey(mGoogleExportWellsHelpID)))
{
    selfld_ = new uiMultiWellSel( this, false );

    uiSeparator* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, selfld_ );
    BufferString flnm = "Wells";
    expfld_ = new uiGISExpStdFld( this, flnm );
    expfld_->attach( stretchedBelow, sep );
    expfld_->attach( leftAlignedBelow, selfld_ );
}


bool uiGISExportWells::acceptOK()
{
    DBKeySet wellids;
    selfld_->getSelected( wellids );
    if ( wellids.isEmpty() )
    {
	uiMSG().error( uiStrings::phrPlsSelectAtLeastOne(uiStrings::sWell()) );
	return false;
    }
    GISWriter* wrr = expfld_->createWriter();
    if ( !wrr )
	return false;
    GISWriter::Property prop;
    prop.xpixoffs_ = 20;
    prop.iconnm_ = "wellpin";
    prop.nmkeystr_ = "Well_Name";
    wrr->setProperties( prop );
    ObjectSet<Pick::Set> picks;
    for ( int idx=0; idx<wellids.size(); idx++ )
    {

	//set color for prop here.
	const DBKey wellid = wellids[idx];
	const Coord coord = Well::MGR().getMapLocation( wellid );
	if ( coord.isUdf() )
	    continue;
	Pick::Set* pick = new Pick::Set( Well::MGR().nameOf( wellid ) );

	Pick::Location loc( coord );
	pick->add( loc );

	picks.add( pick );
    }

    wrr->writePoint( picks );

    wrr->close();

    bool ret = uiMSG().askGoOn( tr("Successfully created %1 for selected wells"
	    " Do you want to create more?" ).arg(wrr->factoryDisplayName()) );

    return !ret;
}
