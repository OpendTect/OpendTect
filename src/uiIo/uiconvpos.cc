/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/

#include "uiconvpos.h"

#include "coordsystem.h"
#include "draw.h"
#include "latlong.h"
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
#include "uilabel.h"
#include "uilistbox.h"
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
    uistrings_ += uiConvertPos::sLLStr();
    uistrings_ += uiConvertPos::sICStr();
    uistrings_ += uiConvertPos::sXYStr();
}


mDefineEnumUtils( uiConvertPos, LatLongType, "Lat-Long Type" )
{
    "Degree",
    "Degree-Minute-Second",
    0
};


template<>
void EnumDefImpl<uiConvertPos::LatLongType>::init()
{
    uistrings_ += ::toUiString("Decimal");
    uistrings_ += ::toUiString("DMS");
}


static const uiString sDMSDescStr()
{
    return od_static_tr( "sDMSDescStr", "DMS format: DDDMMSSSS.SS[H].\n"
			    "H stands for Hemispehere, optional parameter" );
}

static const uiString sDecDescStr()
{
    return od_static_tr( "sDecDescStr", "Dec format: GGG.GGGGH.\n"
			    "H stands for Hemispehere, optional parameter");
}


static const uiString sErrMsg()
{
    return od_static_tr( "sErrMsg", "Input is not correct" );
}


//uiConvPosAscIO
uiConvPosAscIO::uiConvPosAscIO( const Table::FormatDesc& fd, od_istream& strm )
    : Table::AscIO(fd)
    , finishedreadingheader_(false)
    , strm_(strm)
{
}


Table::FormatDesc* uiConvPosAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "ConvPos" );
    fd->bodyinfos_ +=
		    Table::TargetInfo::mkHorPosition( true, true, true, true );
    return fd;
}


uiConvertPos::DataType uiConvPosAscIO::getConvFromTyp()
{
    if ( isXY() )
	return DataSelType::XY;
    else if ( isLL() )
	return DataSelType::LL;
    else
	return DataSelType::IC;
}


bool uiConvPosAscIO::isXY() const
{
    return formOf( false, 0 ) == 0;
}


bool uiConvPosAscIO::isIC() const
{
    return formOf( false, 0 ) == 1;
}


bool uiConvPosAscIO::isLL() const
{
    return formOf( false, 0 ) == 2;
}


bool uiConvPosAscIO::getData( Coord& crd )
{
    if ( !finishedreadingheader_ )
    {
	if ( !getHdrVals(strm_) )
	    return false;

	udfval_ = getFValue( 0 );
	finishedreadingheader_ = true;
    }
    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return false;

    if ( isXY() )
	crd = getPos( 0, 1, udfval_ );
    else if ( isIC() )
    {
	const BinID bid = getBinID( 0, 1, udfval_ );
	crd.x_ = bid.inl();
	crd.y_ = bid.crl();
    }

    return true;
}


//Conversion Position Window
uiConvertPos::uiConvertPos( uiParent* p, const SurveyInfo& si, bool mod )
    : uiDialog(p,uiDialog::Setup(tr("Convert Positions"),
		mNoDlgTitle, mODHelpKey(mConvertPosHelpID) ).modal(mod)
		.oktext(uiString::empty()).canceltext(uiStrings::sClose()))
{
    tabstack_ = new uiTabStack( this, "Tab" );

    mangrp_ = new uiManualConvGroup( tabstack_->tabGroup(), si );
    filegrp_ = new uiFileConvGroup( tabstack_->tabGroup(), si );
    tabstack_->addTab( mangrp_, tr("Single Position") );
    tabstack_->addTab( filegrp_, tr("File") );
}


uiConvertPos::~uiConvertPos()
{
    detachAllNotifiers();
}


//MANUAL CONVERSION GROUP
uiManualConvGroup::uiManualConvGroup( uiParent* p, const SurveyInfo& si )
    :uiDlgGroup(p,tr("Manual Conversion"))
    , survinfo_(si)
    , inpcrdsysselfld_(0)
    , outcrdsysselfld_(0)
{
    uiGroup* topgrp = new uiGroup( this, "TopGroup" );
    topgrp->setVSpacing( 10 );

    inptypfld_ = new uiGenInput( topgrp, tr("Convert From"),
		    StringListInpSpec(uiConvertPos::DataTypeDef().strings()) );
    mAttachCB( inptypfld_->valuechanged, uiManualConvGroup::inputTypChg );

    lltypfld_ = new uiGenInput( topgrp, tr("Lat/Long Format"),
		StringListInpSpec(uiConvertPos::LatLongTypeDef().strings()) );
    lltypfld_->setElemSzPol( uiObject::Small );
    lltypfld_->attach( rightTo, inptypfld_ );
    lltypfld_->setToolTip( sDMSDescStr() );
    mAttachCB( lltypfld_->valuechanged, uiManualConvGroup::llFormatTypChg );
    
    towgs84fld_ = new uiCheckBox( topgrp, tr("Output to WGS84 CRS") );
    towgs84fld_->setChecked( false );
    towgs84fld_->attach( rightOf, lltypfld_ );

    leftinpfld_ = new uiGenInput( topgrp, uiString::empty() );
    leftinpfld_->attach( alignedBelow, inptypfld_ );

    rightinpfld_ = new uiGenInput( topgrp, uiString::empty() );
    rightinpfld_->attach( rightOf, leftinpfld_ );

    const bool isprojcrs = SI().getCoordSystem()->isProjection();

    if ( isprojcrs )
    {
	inpcrdsysselfld_ = new Coords::uiCoordSystemSel( topgrp, true, true,
			SI().getCoordSystem(), tr("Input Coordinate System") );
	inpcrdsysselfld_->attach( alignedBelow, leftinpfld_ );
    }
    uiSeparator* sep = new uiSeparator( this, "Inp-Out Sep" );
    sep->setStretch( 2, 2 );
    sep->attach( stretchedBelow, topgrp );

    uiGroup* botgrp = new uiGroup( this, "BottomGroup" );
    botgrp->setVSpacing( 10 );
    botgrp->attach( alignedBelow, topgrp );
    botgrp->attach( ensureBelow, sep );

    leftoutfld1_ = new uiGenInput( botgrp, uiConvertPos::sLLStr() );
    leftoutfld1_->setReadOnly();

    if ( isprojcrs )
    {
	outcrdsysselfld_ = new Coords::uiCoordSystemSel( botgrp, true, true,
			SI().getCoordSystem(), tr("Output Coordinate System") );
	sep->attach( alignedAbove, outcrdsysselfld_ );
	leftoutfld1_->attach( alignedBelow, outcrdsysselfld_ );
    }
    else
	sep->attach( alignedAbove, leftoutfld1_ );

    rightoutfld1_ = new uiGenInput( botgrp, uiString::empty() );
    rightoutfld1_->setReadOnly();
    rightoutfld1_->attach( rightOf, leftoutfld1_ );

    lltypelbl_ = new uiLabel( botgrp,
			uiConvertPos::LatLongTypeDef().getUiStringForIndex(0) );
    lltypelbl_->attach( rightOf, rightoutfld1_ );

    leftoutfld2_ = new uiGenInput( botgrp, uiConvertPos::sXYStr() );
    leftoutfld2_->setReadOnly();
    leftoutfld2_->attach( alignedBelow, leftoutfld1_ );

    rightoutfld2_ = new uiGenInput( botgrp, uiString::empty() );
    rightoutfld2_->setReadOnly();
    rightoutfld2_->attach( rightOf, leftoutfld2_ );

    leftoutfld3_ = new uiGenInput( botgrp, uiConvertPos::sICStr() );
    leftoutfld3_->setReadOnly();
    leftoutfld3_->attach( alignedBelow, leftoutfld2_ );

    rightoutfld3_ = new uiGenInput( botgrp, uiString::empty() );
    rightoutfld3_->setReadOnly();
    rightoutfld3_->attach( rightOf, leftoutfld3_ );

    uiSeparator* lowersep = new uiSeparator( this, "LoweSep" );
    lowersep->setStretch( 2, 2 );
    lowersep->attach( stretchedBelow, botgrp );

    convertbut_ = new uiPushButton( this, uiStrings::sConvert(), true );
    convertbut_->attach( centeredBelow, botgrp );
    convertbut_->attach( ensureBelow, lowersep );
    mAttachCB( convertbut_->activated, uiManualConvGroup::convButPushCB );

    inputTypChg( 0 );
    llFormatTypChg( 0 );
}

uiManualConvGroup::~uiManualConvGroup()
{
    detachAllNotifiers();
}


void uiManualConvGroup::inputTypChg( CallBacker* )
{
    const int selval = inptypfld_->getIntValue();
    leftinpfld_->setTitleText(
		    uiConvertPos::DataTypeDef().getUiStringForIndex(selval) );
    if ( inpcrdsysselfld_ )
	inpcrdsysselfld_->setSensitive( selval != DataSelType::IC );
    lltypfld_->display( selval == DataSelType::LL );
    lltypelbl_->display( selval == DataSelType::LL );
    leftoutfld3_->display( selval != DataSelType::IC );
    rightoutfld3_->display( selval != DataSelType::IC );
}


void uiManualConvGroup::llFormatTypChg( CallBacker* )
{
    const int selval = lltypfld_->getIntValue();
    lltypelbl_->setText(
		   uiConvertPos::LatLongTypeDef().getUiStringForIndex(selval) );
    lltypelbl_->setToolTip( selval ? sDMSDescStr() : sDecDescStr() );
    lltypfld_->setToolTip( selval ? sDMSDescStr() : sDecDescStr() );
}


void uiManualConvGroup::convButPushCB( CallBacker* )
{
    const int selval = inptypfld_->getIntValue();

    if ( selval == DataSelType::LL )
	convFromLL();
    else if ( selval == DataSelType::IC )
	convFromIC();
    else
	convFromXY();
}

/*
idx=1 : LL
idx=2 : XY
idx=3 : IC
*/

#define mSetOutVal( idx, leftoutval, rightoutval ) \
{ \
    if ( !leftoutfld##idx##_ || !rightoutfld##idx##_ ) \
	pErrMsg("Field Does Not Exist"); \
    leftoutfld##idx##_->setValue( leftoutval ); \
    rightoutfld##idx##_->setValue( rightoutval ); \
} \


#define mErrRet \
{ \
    uiMSG().error( sErrMsg() ); \
    return; \
} \


//TODO : LAT/LNG <--> LAT/LNG CONVERSION NEEDS BETTER IMPLEMENTATION


void uiManualConvGroup::convFromLL()
{
    LatLong ll;
    ll.setFromString( leftinpfld_->text(), true );
    ll.setFromString( rightinpfld_->text(), false  );
    if ( !ll.isDefined() )
	uiMSG().error( ll.errMsg() );

    Coord coord( LatLong::transform( ll, towgs84fld_->isChecked(),
	outcrdsysselfld_->getCoordSystem() ) );

    LatLong outll = LatLong::transform( coord, towgs84fld_->isChecked() );
    mSetOutVal( 1, outll.lat_, outll.lng_ )

    mSetOutVal( 2, coord.x_, coord.y_ )

    BinID bid( SI().transform( coord ) );
    mSetOutVal( 3, bid.inl(), bid.crl() )
}


void uiManualConvGroup::convFromIC()
{
    const BinID bid( leftinpfld_->getFValue(), rightinpfld_->getFValue() );
    if ( bid.isUdf() )
	mErrRet

    const Coord coord( SI().transform( bid ) );

    const LatLong ll( LatLong::transform(coord, towgs84fld_->isChecked(),
					outcrdsysselfld_->getCoordSystem()) );
    mSetOutVal( 1, ll.lat_, ll.lng_ )

    const Coord outcrd = outcrdsysselfld_->getCoordSystem()->convertFrom(
				coord, *inpcrdsysselfld_->getCoordSystem() );
    mSetOutVal( 2, outcrd.x_, outcrd.y_ )
}


void uiManualConvGroup::convFromXY()
{
    const Coord coord( leftinpfld_->getFValue(), rightinpfld_->getFValue() );

    if (coord.isUdf())
	mErrRet

    const LatLong ll( LatLong::transform(coord, towgs84fld_->isChecked(),
					outcrdsysselfld_->getCoordSystem()) );
    mSetOutVal( 1, ll.lat_, ll.lng_ )

    const Coord outcrd = outcrdsysselfld_->getCoordSystem()->convertFrom(
	    coord, *inpcrdsysselfld_->getCoordSystem() );
    mSetOutVal( 2, outcrd.x_, outcrd.y_ )

    const BinID bid( survinfo_.transform( coord ) );
    mSetOutVal( 3, bid.inl(), bid.crl() )
}


#define mErrRetWithMsg(msg) \
{ \
    uiMSG().error( msg ); \
    return; \
} \

//FILE CONVERSION GROUP
//Commented lines have to be implemented.
uiFileConvGroup::uiFileConvGroup( uiParent* p, const SurveyInfo& si )
    :uiDlgGroup(p,tr("File Conversion"))
    , fd_(uiConvPosAscIO::getDesc())
    , survinfo_(si)
    , ostream_(0)
{
    uiGroup* topgrp = new uiGroup( this, "Top Group" );

    uiFileSel::Setup fssu;
    fssu.withexamine( true ).examstyle( File::Table );
    inpfilefld_ = new uiFileSel( topgrp,
		uiStrings::phrInput( uiStrings::sFile() ), fssu );

    

    dataselfld_ = new uiTableImpDataSel( topgrp, *fd_,
				mODHelpKey( mTableImpDataSelwellsHelpID ) );
    dataselfld_->attach( alignedBelow, inpfilefld_ );
    mAttachCB( dataselfld_->descChanged, uiFileConvGroup::inpFileSpecChg );

    uiSeparator* sep = new uiSeparator( this, "Inp-Out Sep" );
    sep->attach( stretchedBelow, topgrp );

    uiGroup* botgrp = new uiGroup( this, "BottomGroup" );
    botgrp->attach( alignedBelow, topgrp );
    botgrp->attach( ensureBelow, sep );

    /*outmodefld_ = new uiGenInput( botgrp, tr("Output mode"),
		BoolInpSpec(true, tr("Add columns"), tr("Replace columns")) );
    outmodefld_->attach( alignedBelow, sep );
    mAttachCB( outmodefld_->valuechanged, uiFileConvGroup::outModeChg );
    */
    insertpos_ = new uiGenInput( botgrp, tr("Insert at"),
				BoolInpSpec(true,tr("Beginning"),tr("End")) );
    insertpos_->attach( alignedBelow, sep );

    towgs84fld_ = new uiCheckBox( botgrp, tr("Output to WGS84 CRS") );
    towgs84fld_->setChecked( false );
    towgs84fld_->attach( rightOf, insertpos_ );

    uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Output types"),
						uiListBox::LblPos::LeftMid );
    su.prefnrlines( uiConvertPos::DataTypeDef().size()+1 );
    outtypfld_ = new uiListBox( botgrp, su );
    outtypfld_->addItems( uiConvertPos::DataTypeDef().strings() );
    outtypfld_->attach( alignedBelow, insertpos_ );
    mAttachCB( outtypfld_->itemChosen, uiFileConvGroup::outTypChg );

    outcrdsysselfld_ = new Coords::uiCoordSystemSel( botgrp, true, true,
			SI().getCoordSystem(), tr("Output Coordinate System") );
    outcrdsysselfld_->attach( alignedBelow, outtypfld_ );

    fssu.setForWrite();
    outfilefld_ = new uiFileSel( botgrp,
			    uiStrings::phrOutput(uiStrings::sFile()), fssu );
    outfilefld_->attach( alignedBelow, outcrdsysselfld_ );

    uiSeparator* lowersep = new uiSeparator( this, "LoweSep" );
    lowersep->attach( stretchedBelow, botgrp );

    convertbut_ = new uiPushButton( this, uiStrings::sConvert(), true );
    convertbut_->attach( centeredBelow, botgrp );
    convertbut_->attach( ensureBelow, lowersep );
    mAttachCB( convertbut_->activated, uiFileConvGroup::convButPushCB );

    //outModeChg( 0 );
}


uiFileConvGroup::~uiFileConvGroup()
{
    detachAllNotifiers();
    delete ostream_;
}


void uiFileConvGroup::outTypChg( CallBacker* )
{
    TypeSet<int> selidxs;
    outtypfld_->getChosen( selidxs );
    if ( selidxs.size() == 1 && uiConvertPos::DataTypeDef()
		    .getEnumForIndex( selidxs.get( 0 ) ) == DataSelType::IC)
	outcrdsysselfld_->display( false );
    else
	outcrdsysselfld_->display( true );

}

/*void uiFileConvGroup::outModeChg( CallBacker* )
{
    const bool isaddmode = outmodefld_->getBoolValue();
    insertpos_->display( isaddmode );

    if ( isaddmode )
	outtypfld_->setChoiceMode( OD::ChooseAtLeastOne );
    else
	outtypfld_->setChoiceMode( OD::ChooseOnlyOne );
}*/


void uiFileConvGroup::llFormatTypChg( CallBacker* )
{
    const int selval = lltypfld_->getIntValue();
    lltypfld_->setToolTip( selval ? sDMSDescStr() : sDecDescStr() );
}


void uiFileConvGroup::inpFileSpecChg( CallBacker* )
{
    od_istream istream("");
    uiConvPosAscIO aio( *fd_, istream );

    uiStringSet outnms;
    outtypfld_->setEmpty();
    outdisptypidxs_.setEmpty();

    for ( int idx = 0; idx<uiConvertPos::DataTypeDef().size(); idx++ )
    {
	if ( idx == DataSelType::IC &&
				    aio.getConvFromTyp() == DataSelType::IC )
	    continue;

	outnms.add( uiConvertPos::DataTypeDef().getUiStringForIndex( idx ) );
	outdisptypidxs_ += idx;
    }

    outtypfld_->addItems( outnms );
}


void uiFileConvGroup::convButPushCB( CallBacker* )
{
    const BufferString inpfnm = inpfilefld_->fileName();

    od_istream istream( inpfnm );
    if ( !istream.isOK() )
	mErrRetWithMsg( uiStrings::phrCannotOpenInpFile() );

    const BufferString outfnm = outfilefld_->fileName();

    delete ostream_;
    ostream_ = new od_ostream( outfnm );
    if ( !ostream_->isOK() )
    {
	deleteAndZeroPtr( ostream_ );
	mErrRetWithMsg( uiStrings::phrCannotOpenOutpFile() );
    }

    uiConvPosAscIO aio( *fd_, istream );
    const uiConvertPos::DataType fromdatatype = aio.getConvFromTyp();
    Coord crd;
    bool convtoxy = false; bool convtoic = false; bool convtoll = false;
    TypeSet<int> idxs;
    outtypfld_->getChosen( idxs );
    for ( int idx=0; idx<idxs.size(); idx++ )
    {
	if ( outdisptypidxs_.get( idxs.get(idx) ) == DataSelType::XY )
	    convtoxy = true;
	else if (outdisptypidxs_.get( idxs.get( idx ) ) == DataSelType::LL )
	    convtoll = true;
	else
	    convtoic = true;
    }

    Coord outcrd;
    LatLong outll;
    BinID outbid;
    od_istream* inpstream = new od_istream( inpfnm );

    const bool towgs = towgs84fld_->isChecked();
    while ( aio.getData(crd) )
    {
	const double firstinp = crd.x_;
	const double secondinp = crd.y_;

	if ( fromdatatype == DataSelType::LL )
	{
	    const LatLong ll( toString(firstinp), toString(secondinp) );
	    if ( !ll.isDefined() )
		continue;

	    Coord coord( LatLong::transform(ll) );
	    outcrd = Coords::CoordSystem::convert( coord,
		*SI().getCoordSystem(), *outcrdsysselfld_->getCoordSystem() );

	    outll = LatLong::transform( coord, towgs );

	    outbid = SI().transform( coord );
	}
	else if ( fromdatatype == DataSelType::IC )
	{
	    const BinID bid( firstinp, secondinp );
	    if ( bid.isUdf() )
		continue;

	    outcrd = SI().transform( bid );

	    outll = LatLong::transform( outcrd, towgs,
				    outcrdsysselfld_->getCoordSystem() );

	    outcrd = outcrdsysselfld_->getCoordSystem()->convertFrom(
		    outcrd, *SI().getCoordSystem() );
					    //get coordsystem from ascio
	}
	else
	{
	    const Coord coord( firstinp, secondinp );
		    //this coord is once converted from File CRS to SI().CRS

	    outll = LatLong::transform( coord, towgs,
					outcrdsysselfld_->getCoordSystem() );

	    outcrd = outcrdsysselfld_->getCoordSystem()->convertFrom(
		outcrd, *SI().getCoordSystem() );

	    outbid = SI().transform( coord );
	}

	//const bool addcol = outmodefld_->getBoolValue();
	const bool addcol = true;
	const bool convdatainbeg  = insertpos_->getBoolValue();

	BufferString filestr;
	inpstream->getLine( filestr );

	if ( addcol && !convdatainbeg )
	    ostream_->add( filestr ).add( od_tab );

	if ( addcol )
	{
	    if ( convtoxy )
		ostream_->add( outcrd.x_ ).add( od_tab ).add( outcrd.y_ )
								.add( od_tab );

	    if ( convtoll )
		ostream_->add( outll.lat_ ).add( od_tab ).add( outll.lng_ )
								.add( od_tab );

	    if ( convtoic )
		ostream_->add( outbid.inl() ).add( od_tab ).add( outbid.crl() )
								.add( od_tab );
	}

	if ( convdatainbeg )
	    ostream_->add( filestr );

	ostream_->add("\n");
    }

    ostream_->close();

    delete inpstream;

    uiMSG().message( tr("File written successfuly") );
}
