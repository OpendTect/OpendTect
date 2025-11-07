/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigisexpdlgs.h"

#include "giswriter.h"
#include "od_helpids.h"
#include "pickset.h"
#include "survinfo.h"

#include "uilistbox.h"
#include "uigeninput.h"
#include "uigisexp.h"
#include "uiioobjselgrp.h"
#include "uimsg.h"
#include "uisellinest.h"
#include "uiseparator.h"


// uiGISExportSurvey

uiGISExportSurvey::uiGISExportSurvey( uiParent* p, const SurveyInfo& si )
    : uiDialog(p,Setup(uiStrings::phrExport(tr("Survey boundaries to GIS")),
		       tr("Specify output parameters"),
		       mODHelpKey(mGoogleExportSurveyHelpID)))
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
    flnm.add( si_.diskLocation().dirName() );

    expfld_ = new uiGISExpStdFld( this, flnm );
    expfld_->attach( stretchedBelow, sep );
    expfld_->attach( leftAlignedBelow, hghtfld_ );
}


uiGISExportSurvey::~uiGISExportSurvey()
{
}


bool uiGISExportSurvey::acceptOK( CallBacker* )
{
    const auto inlrg = si_.inlRange();
    const auto crlrg = si_.crlRange();
    TypeSet<Coord3> coords;
    coords += Coord3( si_.transform(BinID(inlrg.start_,crlrg.start_)), 4 );
    coords += Coord3( si_.transform(BinID(inlrg.start_,crlrg.stop_)), 4 );
    coords += Coord3( si_.transform(BinID(inlrg.stop_,crlrg.stop_)), 4 );
    coords += Coord3( si_.transform(BinID(inlrg.stop_,crlrg.start_)), 4 );
    coords += Coord3( si_.transform(BinID(inlrg.start_,crlrg.start_)), 4 );

    PtrMan<GIS::Writer> wrr = expfld_->createWriter( si_.name().buf(),
						     sKey::Survey() );
    if ( !wrr )
	return false;

    GIS::Property props;
    wrr->getDefaultProperties( GIS::FeatureType::Polygon, props );
    props.linestyle_.color_ = lsfld_->getColor();
    props.linestyle_.width_ = mNINT32( lsfld_->getWidth() * 0.1f );
    props.nmkeystr_ = "SURVEY_ID";
    wrr->setProperties( props );
    if ( !wrr->writePolygon(coords,si_.name()) )
    {
	uiMSG().error( wrr->errMsg() );
	return false;
    }

    const bool ret = uiMSG().askGoOn( wrr->successMsg() );
    return !ret;
}


// uiGISExpPolygon

mDefineEnumUtils(uiGISExportDlg,Type,"Data type")
{
    sKey::PointSet(),
    sKey::Polygon(),
    sKey::RandomLine(),
    sKey::Line(),
    nullptr
};


template <>
void EnumDefImpl<uiGISExportDlg::Type>::init()
{
    uistrings_ += uiStrings::sPointSet(mPlural);
    uistrings_ += uiStrings::sPolygon(mPlural);
    uistrings_ += uiStrings::sRandomLine(mPlural);
    uistrings_ += uiStrings::sLine(mPlural);
}


uiString uiGISExportDlg::getDlgTitle( Type typ )
{
    uiString title;
    switch( typ )
    {
	case Type::PointSet:	title = uiStrings::sPointSet();		break;
	case Type::Polygon:	title = uiStrings::sPolygon();		break;
	case Type::RandomLine:	title = uiStrings::sRandomLine();	break;
	case Type::Line2D:	title = uiStrings::sLine();		break;
    }

    return uiStrings::phrExport( tr("%1 to GIS").arg(title) );
}


int uiGISExportDlg::getHelpKey( Type typ )
{
    switch( typ )
    {
	case Type::PointSet:	return mGoogleExportPolygonHelpID;
	case Type::Polygon:	return mGoogleExportPolygonHelpID;
	case Type::RandomLine:	return mGoogleExportRandomLineHelpID;
	case Type::Line2D:	return mGoogleExport2DSeisHelpID;
	default:		return mGoogleExportPolygonHelpID;
    }
}


uiGISExportDlg::uiGISExportDlg( uiParent* p, Type typ,
				const ObjectSet<const Pick::Set>& gisdatas )
    : uiDialog(p,Setup(getDlgTitle(typ),mODHelpKey(getHelpKey(typ))))
    , typ_(typ)
    , data_(gisdatas)
{
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );

    selfld_ = new uiListBox( this, setup().dlgtitle_, OD::ChooseAtLeastOne );
    fillNames();

    if ( typ != Type::PointSet )
    {
	const OD::LineStyle ls( OD::LineStyle::Solid,
				uiGISExpStdFld::sDefLineWidth(),
				uiGISExpStdFld::sDefColor() );
	uiSelLineStyle::Setup lssu;
	const bool needcolor = typ == Type::RandomLine || typ == Type::Line2D;
	lssu.color( needcolor ).drawstyle( false );
	lsfld_ = new uiSelLineStyle( this, ls, lssu );
	lsfld_->attach( alignedBelow, selfld_ );
    }

    auto* sep = new uiSeparator( this );
    if ( lsfld_ )
	sep->attach( stretchedBelow, lsfld_ );
    else
	sep->attach( stretchedBelow, selfld_ );

    BufferString typstr;
    if ( typ == Type::PointSet )
	typstr = "PointSet";
    else if ( typ == Type::Polygon )
	typstr = "Polygon";
    else if ( typ == Type::RandomLine )
	typstr = "RandomLine";
    else if ( typ == Type::Line2D )
	typstr = "2DLines";

    expfld_ = new uiGISExpStdFld( this, typstr.str() );
    expfld_->attach( ensureBelow, sep );
    if ( lsfld_ )
	expfld_->attach( alignedBelow, lsfld_ );
    else
	expfld_->attach( alignedBelow, selfld_ );
}


uiGISExportDlg::~uiGISExportDlg()
{
    detachAllNotifiers();
}


void uiGISExportDlg::set( const ObjectSet<const Pick::Set>& gisdatas )
{
    selfld_->setEmpty();
    data_ = gisdatas;
    fillNames();
}


void uiGISExportDlg::fillNames()
{
    int idx = 0;
    for ( const auto* gisdata : data_ )
    {
	selfld_->addItem( gisdata->name().buf() );
	bool ison = true, yn = true;
	if ( gisdata->pars_.getYN(sKeyIsOn(),yn) )
	    ison = yn;

	selfld_->setChosen( idx++, ison );
    }
}


bool uiGISExportDlg::acceptOK( CallBacker* )
{
    if ( selfld_->nrChosen() < 1 )
    {
	uiMSG().error( uiStrings::phrPlsSelectAtLeastOne(setup().dlgtitle_) );
	return false;
    }

    const BufferString elemtype = toString( typ_ );
    PtrMan<GIS::Writer> wrr = expfld_->createWriter( SI().name().buf(),
						     elemtype.str() );
    if ( !wrr )
	return false;

    uiRetVal uirv;
    int idx = 0;
    for ( const auto* gisdata : data_ )
    {
	if ( !selfld_->isChosen(idx++) )
	    continue;

	const bool ismulti = typ_ == Type::PointSet || gisdata->nrSets() > 1;
	GIS::FeatureType typ = GIS::FeatureType::Undefined;
	if ( typ_ == Type::PointSet )
	    typ = GIS::FeatureType::MultiPoint;
	else if ( typ_ == Type::Polygon )
	    typ = ismulti ? GIS::FeatureType::MultiPolygon
			  : GIS::FeatureType::Polygon;
	else
	    typ = ismulti ? GIS::FeatureType::MultiLineString
			  : GIS::FeatureType::LineString;

	GIS::Property props;
	wrr->getDefaultProperties( typ, props );
	if ( props.isPoint() )
	{
	    const OD::Color& color = gisdata->disp2d().color();
	    if ( color != OD::Color::NoColor() )
		props.color_ = color;
	    if ( lsfld_ )
	    {
		props.color_ = lsfld_->getColor();
		props.pixsize_ = lsfld_->getWidth() * 0.1;
	    }
	}
	else
	{
	    const bool ispoly = gisdata->isPolygon() &&
				gisdata->disp2d().polyDisp();
	    const OD::LineStyle& linestyle = ispoly
				    ? gisdata->disp2d().polyDisp()->linestyle_
				    : OD::LineStyle();
	    const OD::Color& color = linestyle.color_;
	    if ( color != OD::Color::NoColor() )
		props.linestyle_.color_ = color;

	    const int& linewidth = linestyle.width_;
	    if ( !mIsUdf(linewidth) )
		props.linestyle_.width_ = linewidth;

	    if ( lsfld_ )
	    {
		props.linestyle_.color_ = lsfld_->getColor();
		props.linestyle_.width_ = lsfld_->getWidth() * 0.1;
	    }
	}

	wrr->setProperties( props );

	bool isok;
	if ( props.isPoint() )
	    isok = wrr->writePoints( *gisdata );
	else if ( props.isLine() )
	    isok = ismulti ? wrr->writePolygons( *gisdata )
			   : wrr->writePolygon( *gisdata );
	else
	    isok = ismulti ? wrr->writeLines( *gisdata )
			   : wrr->writeLine( *gisdata );

	if ( !isok )
	    uirv.add( wrr->errMsg() );
    }

    if ( !uirv.isOK() )
    {
	uiMSG().error( uirv );
	return false;
    }

    const bool ret = uiMSG().askGoOn( wrr->successMsg() );
    return !ret;
}
