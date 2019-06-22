/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/

#include "uiconvpos.h"

#include "od_helpids.h"
#include "od_iostream.h"
#include "oddirs.h"
#include "tableascio.h"
#include "tabledef.h"
#include "survinfo.h"
#include "uistrings.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uicoordsystem.h"
#include "uitoolbutton.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uiseparator.h"
#include "uitblimpexpdatasel.h"
#include "uimsg.h"


#define mMaxLineBuf 32000

mDefineEnumUtils( uiConvertPos, DataType, "Conversion Type" )
{
    "Lat-Lon",
    "Inl-Crl",
    "X-Y",
    0
};

template<>
void EnumDefImpl<uiConvertPos::DataType>::init()
{
    uistrings_ += mEnumTr("Lat-Lon", 0);
    uistrings_ += mEnumTr("Inl-Crl", 0);
    uistrings_ += mEnumTr("X-Y", 0);
}


Table::FormatDesc* uiConvPosAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "ConvPos" );
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true, true, true,
									true );
    return fd;
}


uiConvertPos::DataType uiConvPosAscIO::getConvFromTyp()
{
    if ( isXY() )
	return uiConvertPos::DataType::XY;
    else if ( isLL() )
	return uiConvertPos::DataType::LL;
    else
	return uiConvertPos::DataType::IC;
}


bool uiConvPosAscIO::isXY() const
{
    return formOf( false, 1 ) == 0;
}


bool uiConvPosAscIO::isIC() const
{
    return formOf( false, 1 ) == 1;
}


bool uiConvPosAscIO::isLL() const
{
    return formOf( false, 1 ) == 2;
}


bool uiConvPosAscIO::getData( Coord& crd )
{
    if ( !finishedreadingheader_ )
    {
	if (!getHdrVals( strm_ ))
	    return false;

	udfval_ = getFValue( 0 );
	finishedreadingheader_ = true;
    }
    crd = getPos( 0, 1, udfval_ );
    return true;
}


uiConvertPos::uiConvertPos( uiParent* p, const SurveyInfo& si, bool mod )
	: uiDialog(p, uiDialog::Setup(tr("Convert Positions"),
		   mNoDlgTitle, mODHelpKey(mConvertPosHelpID) ).modal(mod))
	, survinfo_(si)
	, ostream_(0)
	, fd_(uiConvPosAscIO::getDesc())
{
    manfld_ = new uiGenInput( this, uiStrings::sConversion(),
	           BoolInpSpec(true,uiStrings::sManual(),uiStrings::sFile()) );
    mAttachCB( manfld_->valuechanged, uiConvertPos::selChg );

    inputypfld_ = new uiGenInput( this, uiStrings::phrInput(uiStrings::sType()),
			    StringListInpSpec(DataTypeDef().strings()) );
    inputypfld_->attach( alignedBelow, manfld_ );
    mAttachCB( inputypfld_->valuechanged, uiConvertPos::inputTypChg );

    leftinpfld_ = new uiGenInput( this, ::toUiString("*********************") );
    leftinpfld_->attach( alignedBelow, inputypfld_ );

    rightinpfld_ = new uiGenInput( this, uiString::empty() );
    rightinpfld_->attach( rightOf, leftinpfld_ );

    uiFileSel::Setup fssu;
    fssu.withexamine( true ).examstyle( File::Table );
    inpfilefld_ = new uiFileSel( this, uiStrings::phrInput(uiStrings::sFile()),
									fssu );

    inpfilefld_->attach( alignedBelow, manfld_ );
    inpfilefld_->display( false );
    dataselfld_ = new uiTableImpDataSel( this, *fd_,
				mODHelpKey(mTableImpDataSelwellsHelpID) );
    dataselfld_->attach( alignedBelow, inpfilefld_ );
    inpcrdsysselfld_ = new Coords::uiCoordSystemSel( this, true, true,
				    SI().getCoordSystem(), tr("Input CRS") );
    inpcrdsysselfld_->attach( alignedBelow, leftinpfld_ );

    outputtypfld_ = new uiGenInput( this,
			uiStrings::phrOutput(uiStrings::sType()),
			StringListInpSpec(DataTypeDef().strings()) );
    outputtypfld_->attach( alignedBelow, dataselfld_ );
    mAttachCB( outputtypfld_->valuechanged, uiConvertPos::outputTypChg );

    outcrdsysselfld_ = new Coords::uiCoordSystemSel( this, true, true,
							0, tr( "Output CRS" ) );
    outcrdsysselfld_->attach( alignedBelow, outputtypfld_ );

    leftoutfld_ = new uiGenInput( this, ::toUiString("*********************") );
    leftoutfld_->attach( alignedBelow, outcrdsysselfld_ );
    leftoutfld_->setSensitive( false );

    rightoutfld_ = new uiGenInput( this, uiString::empty() );
    rightoutfld_->attach( rightOf, leftoutfld_ );
    rightoutfld_->setSensitive( false );


    fssu.setForWrite();
    outfilefld_ = new uiFileSel( this, uiStrings::phrOutput(
						    uiStrings::sFile()), fssu );
    outfilefld_->attach( alignedBelow, outcrdsysselfld_ );

    convertbut_ = new uiPushButton( this, uiStrings::sConvert(), 0, true );
    convertbut_->attach( alignedBelow, outfilefld_ );
    mAttachCB( convertbut_->activated, uiConvertPos::convertCB );

    mAttachCB( preFinalise(), uiConvertPos::inputTypChg );
    mAttachCB( postFinalise(), uiConvertPos::selChg );
}

uiConvertPos::~uiConvertPos()
{
    if ( ostream_ )
	delete ostream_;
    detachAllNotifiers();
}


void uiConvertPos::selChg( CallBacker* )
{
    const bool isman = manfld_->getBoolValue();

    outfilefld_->display( !isman );
    inpfilefld_->display( !isman );
    dataselfld_->display( !isman );
    inputypfld_->display( isman );
    inpcrdsysselfld_->display( isman && (inputypfld_->getIntValue() != IC) );
    outcrdsysselfld_->display( isman && (inputypfld_->getIntValue() != IC) );
    leftoutfld_->display( isman );
    rightoutfld_->display( isman );
    leftinpfld_->display( isman );
    rightinpfld_->display( isman );
}


void uiConvertPos::inputTypChg( CallBacker* )
{
    const int selval = inputypfld_->getIntValue();
    leftinpfld_->setTitleText( DataTypeDef().getUiStringForIndex(
								    selval) );
    uiStringSet outtypstrs;
    outidxs_.setEmpty();

    for ( int idx=0; idx<DataTypeDef().size(); idx++ )
    {
	if ( selval == IC && idx == IC )
	    continue;
	outidxs_ += idx;
	outtypstrs.add( DataTypeDef().getUiStringForIndex(idx) );
    }

    inpcrdsysselfld_->setSensitive( selval != IC );

    outputtypfld_->newSpec( StringListInpSpec(outtypstrs), 0 );
    outputtypfld_->updateSpecs();
    outputTypChg(0);
}


void uiConvertPos::outputTypChg( CallBacker* )
{
    const int idx = outidxs_[outputtypfld_->getIntValue()];
    leftoutfld_->setTitleText( DataTypeDef().getUiStringForIndex( idx ) );
}


#define mSetOutVal( outval1, outval2 ) \
{ \
    if ( ismanmode ) \
    { \
	leftoutfld_->setValue( outval1 ); \
	rightoutfld_->setValue( outval2 ); \
    } \
    else \
	*ostream_ << outval1 << ' ' << outval2 << ' ' << linebuf_ << \
								    od_endl; \
} \


uiConvertPos::DataType uiConvertPos::getConversionType()
{
    const int outidx = outputtypfld_->getIntValue();
    return DataTypeDef().getEnumForIndex( outidxs_[outidx] );
}


void uiConvertPos::errMsgNEmpFlds()
{
    uiMSG().error( tr("Cannot convert this position") );
    leftoutfld_->setEmpty(); rightoutfld_->setEmpty();
}


void uiConvertPos::convFromIC( bool ismanmode )
{
    DataType convtotyp = getConversionType();

    BinID bid( firstinp_, secondinp_ );
    if ( bid.isUdf() )
    {
	errMsgNEmpFlds();
	return;
    }

    Coord coord( survinfo_.transform(bid) );

    if ( convtotyp == XY )
	mSetOutVal( coord.x_, coord.y_ )
    else
    {
	const LatLong ll( LatLong::transform(coord, true,
				    outcrdsysselfld_->getCoordSystem()) );
	mSetOutVal( ll.lat_, ll.lng_ )
    }
}


void uiConvertPos::convFromXY( bool ismanmode )
{
    DataType convtotyp = getConversionType();

    Coord coord( firstinp_, secondinp_ );

    if ( coord.isUdf() )
    {
	errMsgNEmpFlds();
	return;
    }

    if ( convtotyp == LL )
    {
	const LatLong ll( LatLong::transform(coord, true,
				    outcrdsysselfld_->getCoordSystem()) );
	mSetOutVal( ll.lat_, ll.lng_ )
    }
    else if ( convtotyp == IC )
    {
	BinID bid( survinfo_.transform(coord) );
	mSetOutVal( bid.inl(), bid.crl() )
    }
    else
    {
	Coord outcrd = outcrdsysselfld_->getCoordSystem()->convertFrom(
	    coord, *inpcrdsysselfld_->getCoordSystem() );
	mSetOutVal( outcrd.x_, outcrd.y_ )
    }
}


void uiConvertPos::convFromLL( bool ismanmode )
{
    DataType convtotyp = getConversionType();

    LatLong ll( firstinp_, secondinp_ );
    if ( !ll.isDefined() )
    {
	errMsgNEmpFlds();
	return;
    }

    Coord coord( LatLong::transform(ll, true,
					outcrdsysselfld_->getCoordSystem()) );

    if ( convtotyp == IC )
    {
	BinID bid( survinfo_.transform(coord) );
	mSetOutVal( bid.inl(), bid.crl() )
    }
    else if ( convtotyp == XY )
	mSetOutVal( coord.x_, coord.y_ )
    else
    {
	LatLong outll = LatLong::transform( coord );
	mSetOutVal( outll.lat_, outll.lng_ )
    }

}

#define mErrRet(s) { uiMSG().error(s); return; }

void uiConvertPos::convertCB( CallBacker* )
{
    if ( manfld_->getBoolValue() )
	convManually();
    else
	convFile();
}


void uiConvertPos::launchSelConv( bool ismanmode, int selidx )
{
    if ( selidx == XY )
	convFromXY( ismanmode );
    else if (selidx == LL)
	convFromLL( ismanmode );
    else
	convFromIC( ismanmode );

}

void uiConvertPos::convManually()
{
    const int selidx = inputypfld_->getIntValue();

    firstinp_ = leftinpfld_->getFValue();
    secondinp_ = rightinpfld_->getFValue();

    launchSelConv( true, selidx );
}


void uiConvertPos::convFile()
{
    const BufferString inpfnm = inpfilefld_->fileName();

    od_istream istream( inpfnm );
    if ( !istream.isOK() )
	mErrRet( uiStrings::phrCannotOpenInpFile() );

    const BufferString outfnm = outfilefld_->fileName();
    ostream_ = new od_ostream( outfnm );
    if ( !ostream_->isOK() )
    {
	delete ostream_;
	mErrRet( uiStrings::phrCannotOpenOutpFile() );
    }

    int nrln = 0;
    uiConvPosAscIO aio( *fd_, istream );
    const int selidx = aio.getConvFromTyp();
    if ( getConversionType() == IC && selidx == IC )
	uiMSG().error( tr("Cannot process. Convert to and from are IC") );
    Coord crd;
    while ( aio.getData( crd ) )
    {
	firstinp_ = crd.x_;
	secondinp_ = crd.y_;

	launchSelConv( false, selidx );
    }

    uiMSG().message( tr("Total number of converted lines: %1")
						    .arg(::toString(nrln)) );
}
