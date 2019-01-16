/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2007
-*/


#include "uigoogleexpsurv.h"
#include "googlexmlwriter.h"
#include "uisurveymanager.h"
#include "uisellinest.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uifiledlg.h"
#include "uimsg.h"
#include "oddirs.h"
#include "strmprov.h"
#include "survinfo.h"
#include "latlong.h"
#include "draw.h"
#include "od_helpids.h"

#include "geojsonwriter.h"

#include "uiseparator.h"
#include "uigoogleexpdlg.h"

#include "uiparent.h"

#include <iostream>


uiGISExportSurvey::uiGISExportSurvey( uiSurveyManager* uisurv )
    : uiDialog(uisurv,
       uiDialog::Setup(uiStrings::phrExport( tr("Survey boundaries to KML")),
		      tr("Specify output parameters"),
		      mODHelpKey(mGoogleExportSurveyHelpID) ) )
    , si_(uisurv->curSurvInfo())
{
    const OD::LineStyle ls( OD::LineStyle::Solid, 20, Color(255,170,80,100) );
    uiSelLineStyle::Setup lssu; lssu.drawstyle( false ).transparency( true );
    lsfld_ = new uiSelLineStyle( this, ls, lssu );

    hghtfld_ = new uiGenInput( this, tr("Border height"), FloatInpSpec(500) );
    hghtfld_->attach( alignedBelow, lsfld_ );
    uiSeparator* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, hghtfld_ );
    BufferString flnm = "SurveyBoudaryFor_";
    flnm.add( SI().dirName() );
	expfld_ = new uiGISExpStdFld( this, flnm );
    expfld_->attach( stretchedBelow, sep );
    expfld_->attach( leftAlignedBelow, hghtfld_ );
}


uiGISExportSurvey::~uiGISExportSurvey()
{
}


bool uiGISExportSurvey::acceptOK()
{
    const auto inlrg = si_->inlRange();
    const auto crlrg = si_->crlRange();
    TypeSet<Coord> coords;
    coords += si_->transform( BinID(inlrg.start,crlrg.start) );
    coords += si_->transform( BinID(inlrg.start,crlrg.stop) );
    coords += si_->transform( BinID(inlrg.stop,crlrg.stop) );
    coords += si_->transform( BinID(inlrg.stop,crlrg.start) );
    coords += si_->transform( BinID(inlrg.start,crlrg.start) );
    Pick::Set* pick = new Pick::Set( "SurveryBoundary" );
    for ( int idx=0; idx<coords.size(); idx++ )
    {
	Pick::Location loc( coords.get(idx), hghtfld_->getFValue() );
	pick->add( loc );
    }

    ObjectSet<Pick::Set> picks;
    picks.add( pick );

    GISWriter* wrr = expfld_->createWriter();
    if ( !wrr )
	return false;
    GISWriter::Property props;
    props.color_ = lsfld_->getColor();
    props.width_ = mNINT32( lsfld_->getWidth() * 0.1f );
    props.stlnm_ = "survey";
    wrr->setProperties( props );
    wrr->writePolygon( picks );
    wrr->close();
    return true;
}
