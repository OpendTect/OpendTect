/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigoogleexpsurv.h"
#include "googlexmlwriter.h"
#include "uisurvey.h"
#include "uisellinest.h"
#include "uigeninput.h"
#include "uifileinput.h"
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


uiGISExportSurvey::uiGISExportSurvey( uiParent* p, SurveyInfo* si )
    : uiDialog(p,
       uiDialog::Setup(uiStrings::phrExport( tr("Survey boundaries to GIS")),
		      tr("Specify output parameters"),
		      mODHelpKey(mGoogleExportSurveyHelpID) ) )
    , si_(si)
{
    const OD::LineStyle ls( OD::LineStyle::Solid, 20,
						    OD::Color(255,170,80,100) );
    uiSelLineStyle::Setup lssu; lssu.drawstyle( false ).transparency( true );
    lsfld_ = new uiSelLineStyle( this, ls, lssu );

    hghtfld_ = new uiGenInput( this, tr("Border height"), FloatInpSpec(500) );
    hghtfld_->attach( alignedBelow, lsfld_ );
    auto* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, hghtfld_ );
    BufferString flnm = "SurveyBoudaryFor_";
    flnm.add( SI().diskLocation().dirName() );
	expfld_ = new uiGISExpStdFld( this, flnm );
    expfld_->attach( stretchedBelow, sep );
    expfld_->attach( leftAlignedBelow, hghtfld_ );
}


uiGISExportSurvey::~uiGISExportSurvey()
{
}


bool uiGISExportSurvey::acceptOK( CallBacker* )
{
    if ( !si_ )
	return false;

    const auto inlrg = si_->inlRange();
    const auto crlrg = si_->crlRange();
    TypeSet<Coord3> coords;
    coords += Coord3( si_->transform(BinID(inlrg.start,crlrg.start)), 4 );
    coords += Coord3( si_->transform(BinID(inlrg.start,crlrg.stop)), 4 );
    coords += Coord3( si_->transform(BinID(inlrg.stop,crlrg.stop)), 4 );
    coords += Coord3( si_->transform(BinID(inlrg.stop,crlrg.start)), 4 );
    coords += Coord3( si_->transform(BinID(inlrg.start,crlrg.start)), 4 );

    PtrMan<GISWriter> wrr = expfld_->createWriter();
    if ( !wrr )
	return false;

    GISWriter::Property props;
    props.color_ = lsfld_->getColor();
    props.width_ = mNINT32( lsfld_->getWidth() * 0.1f );
    props.stlnm_ = "survey";
    props.nmkeystr_ = "SURVEY_ID";
    wrr->setProperties( props );
    if ( !wrr->writePolygon(coords,si_->name()) )
    {
	uiMSG().error( wrr->errMsg() );
	return false;
    }

    wrr->close();
    const bool ret = uiMSG().askGoOn( wrr->successMsg() );

    return !ret;
}
