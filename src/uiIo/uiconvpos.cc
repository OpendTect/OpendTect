/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/

#include "uiconvpos.h"
#include "survinfo.h"
#include "strmprov.h"
#include "od_helpids.h"
#include "od_iostream.h"
#include "oddirs.h"
#include "uistrings.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uicoordsystem.h"
#include "uitoolbutton.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uiseparator.h"
#include "uimsg.h"


#define mMaxLineBuf 32000
static BufferString lastinpfile;
static BufferString lastoutfile;

mDefineEnumUtils( uiConvertPos, ConversionType, "Conversion Type" )
{
    "CRS",
    "Inl-Crl",
    "X-Y",
    "Lat-Lon",
    0
};

template<>
void EnumDefImpl<uiConvertPos::ConversionType>::init()
{
    uistrings_ += mEnumTr("CRS (X-Y)", 0);
    uistrings_ += mEnumTr("Inl-Crl", 0);
    uistrings_ += mEnumTr("X-Y", 0);
    uistrings_ += mEnumTr("Lat-Long", 0);
}


uiConvertPos::uiConvertPos( uiParent* p, const SurveyInfo& si, bool mod )
	: uiDialog(p, uiDialog::Setup(tr("Convert Positions"),
		   mNoDlgTitle, mODHelpKey(mConvertPosHelpID) ).modal(mod))
	, survinfo_(si)
	, ostream_(0)
{
    manfld_ = new uiGenInput( this, uiStrings::sConversion(),
	           BoolInpSpec(true,uiStrings::sManual(),uiStrings::sFile()) );
    mAttachCB( manfld_->valuechanged, uiConvertPos::selChg );

    inputypfld_ = new uiGenInput( this, uiStrings::phrInput(uiStrings::sType()),
			    StringListInpSpec(ConversionTypeDef().strings()) );
    inputypfld_->attach( alignedBelow, manfld_ );
    mAttachCB( inputypfld_->valuechanged, uiConvertPos::inputTypChg );

    leftinpfld_ = new uiGenInput( this, ::toUiString("*********************") );
    leftinpfld_->attach( alignedBelow, inputypfld_ );

    rightinpfld_ = new uiGenInput( this, uiString::empty() );
    rightinpfld_->attach( rightOf, leftinpfld_ );

    uiFileSel::Setup fssu( lastinpfile );
    fssu.withexamine( true ).examstyle( File::Table );
    inpfilefld_ = new uiFileSel( this, uiStrings::phrInput(uiStrings::sFile()),
									fssu );
    inpfilefld_->attach( alignedBelow, inputypfld_ );
    inpfilefld_->display( false );

    inpcrdsysselfld_ = new Coords::uiCoordSystemSel( this, true, true,
				    SI().getCoordSystem(), tr("Input CRS") );
    inpcrdsysselfld_->attach( alignedBelow, leftinpfld_ );

    outputtypfld_ = new uiGenInput( this,
			uiStrings::phrOutput(uiStrings::sType()),
			StringListInpSpec(ConversionTypeDef().strings()) );
    outputtypfld_->attach( alignedBelow, inpcrdsysselfld_ );
    mAttachCB( outputtypfld_->valuechanged, uiConvertPos::outputTypChg );

    leftoutfld_ = new uiGenInput( this, ::toUiString("*********************") );
    leftoutfld_->attach( alignedBelow, outputtypfld_ );
    leftoutfld_->setSensitive( false );

    rightoutfld_ = new uiGenInput( this, uiString::empty() );
    rightoutfld_->attach( rightOf, leftoutfld_ );
    rightoutfld_->setSensitive( false );
    outcrdsysselfld_ = new Coords::uiCoordSystemSel( this, true, true,
						    0, tr("Output CRS") );
    outcrdsysselfld_->attach( alignedBelow, leftoutfld_ );

    fssu.setFileName( lastoutfile );
    fssu.setForWrite();
    outfilefld_ = new uiFileSel( this, uiStrings::phrOutput(
						    uiStrings::sFile()), fssu );
    outfilefld_->attach( alignedBelow, outcrdsysselfld_ );

    convertbut_ = new uiPushButton( this, uiStrings::sConvert(), 0, true );
    convertbut_->attach( alignedBelow, outfilefld_ );
    mAttachCB( convertbut_->activated, uiConvertPos::convertCB );

    mAttachCB( postFinalise(), uiConvertPos::inputTypChg );
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
    leftoutfld_->display( isman );
    rightoutfld_->display( isman );
    leftinpfld_->display( isman );
    rightinpfld_->display( isman );
}


void uiConvertPos::inputTypChg( CallBacker* )
{
    const int selval = inputypfld_->getIntValue();
    leftinpfld_->setTitleText( ConversionTypeDef().getUiStringForIndex(
								    selval) );
    outputtypfld_->setSensitive( true );
    uiStringSet outtypstrs;

    outidxs_.setEmpty();

    if ( selval == CRS )
    {
	outtypstrs.add( ConversionTypeDef().getUiStringForIndex(CRS) );
	outidxs_ += CRS;
    }
    else
    {
	for ( int idx=0; idx<ConversionTypeDef().size(); idx++ )
	{
	    if ( idx == CRS || selval == idx )
		continue;
	    outidxs_ += idx;
	    outtypstrs.add( ConversionTypeDef().getUiStringForIndex(idx) );
	}
    }

    outputtypfld_->newSpec( StringListInpSpec(outtypstrs), 0 );
    outputtypfld_->updateSpecs();
    outputtypfld_->setSensitive( selval != CRS );
    outputTypChg( 0 );
}


void uiConvertPos::outputTypChg( CallBacker* )
{
    const int selval = inputypfld_->getIntValue();
    const int idx = outidxs_[outputtypfld_->getIntValue()];
    leftoutfld_->setTitleText( ConversionTypeDef().getUiStringForIndex(idx) );
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


uiConvertPos::ConversionType uiConvertPos::getConversionType()
{
    const int outidx = outputtypfld_->getIntValue();
    ConversionType convtotyp = ConversionTypeDef().getEnumForIndex(
	outidxs_[outidx] );
    return convtotyp;
}


void uiConvertPos::errMsgNEmpFlds()
{
    uiMSG().error( tr("Cannot convert this position") );
    leftoutfld_->setEmpty(); rightoutfld_->setEmpty();
}


void uiConvertPos::convFromIC( bool ismanmode )
{
    ConversionType convtotyp = getConversionType();

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
    ConversionType convtotyp = getConversionType();

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
    else
    {
	BinID bid( survinfo_.transform(coord) );
	mSetOutVal( bid.inl(), bid.crl() )
    }
}


void uiConvertPos::convFromLL( bool ismanmode )
{
    ConversionType convtotyp = getConversionType();

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
    else
	mSetOutVal( coord.x_, coord.y_ )
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
    if (selidx == XY)
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

    if ( selidx != CRS )
	launchSelConv( true, selidx );
    else
    {
	Coord coord( firstinp_, secondinp_ );
	Coord outcrd = outcrdsysselfld_->getCoordSystem()->convertFrom(
				coord, *inpcrdsysselfld_->getCoordSystem() );
    }
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

    lastinpfile = inpfnm; lastoutfile = outfnm;

    const int selidx = inputypfld_->getIntValue();
    int nrln = 0;
    while ( istream.isOK() )
    {
	istream.get( firstinp_ );
	istream.get( secondinp_ );
	if ( !istream.isOK() )
	    break;

	istream.getLine( linebuf_ );
	if ( istream.isBad() )
	    break;
	if ( selidx != CRS )
	    launchSelConv( false, selidx );
	else
	{
	    Coord coord( firstinp_, secondinp_ );
	    Coord outcrd = outcrdsysselfld_->getCoordSystem()->convertFrom(
		coord, *inpcrdsysselfld_->getCoordSystem() );
	    *ostream_ << outcrd.x_ << ' ' << outcrd.y_ << ' ' << linebuf_ <<
	    od_endl;
	}

	nrln++;
    }

    uiMSG().message( tr("Total number of converted lines: %1")
						    .arg(::toString(nrln)) );
}
