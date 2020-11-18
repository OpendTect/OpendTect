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
#include "uifileinput.h"
#include "uilabel.h"
#include "uilatlonginp.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitblimpexpdatasel.h"
#include "uimsg.h"


#define mMaxLineBuf 32000

mDefineEnumUtils(uiConvertPos, DataType, "Conversion Type")
{
    "X-Y",
    "Inl-Crl",
    "Lat-Lon",
    0
};


template<>
void EnumDefImpl<uiConvertPos::DataType>::init()
{
    uistrings_ += uiConvertPos::sXYStr();
    uistrings_ += uiConvertPos::sICStr();
    uistrings_ += uiConvertPos::sLLStr();
}


mDefineEnumUtils(uiConvertPos, LatLongType, "Lat-Long Type")
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
    return od_static_tr("sDMSDescStr", "DMS format: DDDMMSSSS.SS[H].\n"
	"H stands for Hemispehere, optional parameter");
}

static const uiString sDecDescStr()
{
    return od_static_tr("sDecDescStr", "Dec format: GGG.GGGGH.\n"
	"H stands for Hemispehere, optional parameter");
}


static const uiString sErrMsg()
{
    return od_static_tr("sErrMsg", "Input is not correct");
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
    if ( ret <= 0 )
	return false;

    if ( isXY() )
	crd = getPos( 0, 1, udfval_ );
    else if ( isIC() )
    {
	const BinID bid = getBinID( 0, 1, udfval_ );
	crd.x = bid.inl();
	crd.y = bid.crl();
    }

    return true;
}


//Conversion Position Window
uiConvertPos::uiConvertPos( uiParent* p, const SurveyInfo& si, bool mod )
    : uiDialog(p, uiDialog::Setup(tr("Convert Positions"),
	mNoDlgTitle, mODHelpKey(mConvertPosHelpID)).modal(mod)
	.oktext(uiString::emptyString()).canceltext(uiStrings::sClose()))
{
    tabstack_ = new uiTabStack( this, "Tab" );

    mangrp_ = new uiManualConvGroup( tabstack_->tabGroup(), si );
    filegrp_ = new uiFileConvGroup( tabstack_->tabGroup(), si );
    tabstack_->addTab( mangrp_, tr("Single Position") );
    tabstack_->addTab( filegrp_, uiStrings::sFile() );
}


uiConvertPos::~uiConvertPos()
{
    detachAllNotifiers();
}


// uiManualConvGroup
uiManualConvGroup::uiManualConvGroup( uiParent* p, const SurveyInfo& si )
    :uiDlgGroup(p, tr("Manual Conversion"))
    , inpcrdsysselfld_(nullptr)
    , outcrdsysselfld_(nullptr)
    , lloutfld_(nullptr)
    , survinfo_(si)
{
    uiGroup* ingrp = new uiGroup( this, "TopGroup" );

    inptypfld_ = new uiGenInput( ingrp, tr("Convert From"),
	StringListInpSpec(uiConvertPos::DataTypeDef().strings()) );
    mAttachCB( inptypfld_->valuechanged, uiManualConvGroup::inputTypChg );
    ingrp->setHAlignObj( inptypfld_ );

    uiObject* attachobj = inptypfld_->attachObj();
    const bool isprojcrs = si.getCoordSystem()->isProjection();
    if ( isprojcrs )
    {
	inpcrdsysselfld_ = new Coords::uiCoordSystemSel( ingrp, true, true,
	    si.getCoordSystem(), tr("Input Coordinate System") );
	inpcrdsysselfld_->attach( alignedBelow, inptypfld_ );
	attachobj = inpcrdsysselfld_->attachObj();
    }

    xyinfld_ = new uiGenInput( ingrp, uiConvertPos::sXYStr(),
				DoubleInpSpec(), DoubleInpSpec() );
    xyinfld_->setElemSzPol( uiObject::Medium );
    xyinfld_->attach( alignedBelow, attachobj );

    inlcrlinfld_ = new uiGenInput( ingrp, uiConvertPos::sICStr(),
				   IntInpSpec(), IntInpSpec() );
    inlcrlinfld_->setElemSzPol( uiObject::Medium );
    inlcrlinfld_->attach( alignedBelow, inptypfld_ );

    llinfld_ = new uiLatLongInp( ingrp );
    llinfld_->attach( alignedBelow, inptypfld_ );

    convertbut_ = new uiPushButton( this, uiStrings::sConvert(), true );
    convertbut_->attach( centeredBelow, ingrp );
    mAttachCB(convertbut_->activated,uiManualConvGroup::convButPushCB);

    towgs84fld_ = new uiCheckBox( this, tr("Output to WGS84 CRS") );
    towgs84fld_->setChecked( false );
    towgs84fld_->attach( rightOf, convertbut_ );

    uiSeparator* sep = new uiSeparator( this, "Inp-Out Sep" );
    sep->setStretch( 2, 0 );
    sep->attach( stretchedBelow, convertbut_ );

    uiGroup* outgrp = new uiGroup( this, "BottomGroup" );
    outgrp->attach( alignedBelow, ingrp );
    outgrp->attach( ensureBelow, sep );
    outgrp->setVSpacing( 10 );

    if ( isprojcrs )
    {
	outcrdsysselfld_ = new Coords::uiCoordSystemSel( outgrp, true, true,
	    si.getCoordSystem(), tr("Output Coordinate System") );
    }

    xyoutfld_ = new uiGenInput( outgrp, uiConvertPos::sXYStr(),
					DoubleInpSpec(), DoubleInpSpec() );
    xyoutfld_->setElemSzPol( uiObject::Medium );
    xyoutfld_->setReadOnly( true );
    outgrp->setHAlignObj( xyoutfld_ );
    if ( outcrdsysselfld_ )
	xyoutfld_->attach( alignedBelow, outcrdsysselfld_ );

    inlcrloutfld_ = new uiGenInput( outgrp, uiConvertPos::sICStr(),
				    IntInpSpec(), IntInpSpec() );
    inlcrloutfld_->setElemSzPol( uiObject::Medium );
    inlcrloutfld_->setReadOnly( true );
    inlcrloutfld_->attach( alignedBelow, xyoutfld_ );

    if ( inpcrdsysselfld_ )
    {
	lloutfld_ = new uiLatLongInp( outgrp );
	lloutfld_->attach( alignedBelow, inlcrloutfld_ );
    }

    inputTypChg( 0 );
}


uiManualConvGroup::~uiManualConvGroup()
{
    detachAllNotifiers();
}


void uiManualConvGroup::inputTypChg( CallBacker* )
{
    const int selval = inptypfld_->getIntValue();
    xyinfld_->display( selval==DataSelType::XY );
    inlcrlinfld_->display( selval==DataSelType::IC );
    llinfld_->display( selval==DataSelType::LL );
    if ( inpcrdsysselfld_ )
	inpcrdsysselfld_->display( selval==DataSelType::XY );
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

    xyoutfld_->setNrDecimals( SI().nrXYDecimals(), 0 );
    xyoutfld_->setNrDecimals( SI().nrXYDecimals(), 1 );
}


#define mErrRet(msg) \
{ \
    uiMSG().error( msg ); \
    return; \
} \

//TODO : LAT/LNG <--> LAT/LNG CONVERSION NEEDS BETTER IMPLEMENTATION
void uiManualConvGroup::convFromLL()
{
    if ( !inpcrdsysselfld_ )
    {
	mErrRet( tr("No CRS has been set for this survey.\n"
		    "Please provide a CRS in your survey settings.") );
    }

    LatLong ll;
    llinfld_->get( ll );
    if ( !ll.isDefined() )
	mErrRet( tr("Lat-Long value not defined") )

    Coord incoord;
    if ( inpcrdsysselfld_ )
	incoord = LatLong::transform( ll, towgs84fld_->isChecked(),
				      survinfo_.getCoordSystem() );

    Coord outcoord;
    if ( outcrdsysselfld_ )
	outcoord = LatLong::transform( ll, towgs84fld_->isChecked(),
				    outcrdsysselfld_->getCoordSystem() );

    const LatLong outll = LatLong::transform( incoord,
					      towgs84fld_->isChecked() );
    const BinID outbid( survinfo_.transform(incoord) );

    xyoutfld_->setValue( outcoord );
    inlcrloutfld_->setValue( outbid );
    if ( lloutfld_ )
	lloutfld_->set( outll );
}


void uiManualConvGroup::convFromIC()
{
    const BinID bid = inlcrlinfld_->getBinID();
    if ( bid.isUdf() )
	mErrRet( sErrMsg() )

    const Coord coord( survinfo_.transform(bid) );

    const LatLong outll = LatLong::transform( coord, towgs84fld_->isChecked(),
					      survinfo_.getCoordSystem() );

    Coord outcrd = coord;
    if ( outcrdsysselfld_ )
	outcrd = outcrdsysselfld_->getCoordSystem()->convertFrom(
				coord, *survinfo_.getCoordSystem() );

    xyoutfld_->setValue( outcrd );
    inlcrloutfld_->setValue( bid );
    if ( lloutfld_ )
	lloutfld_->set( outll );
}


void uiManualConvGroup::convFromXY()
{
    const Coord coord = xyinfld_->getCoord();
    if ( coord.isUdf() )
	mErrRet( sErrMsg() )

    Coord survcoord = coord;
    LatLong outll;
    if ( inpcrdsysselfld_ )
    {
	outll = LatLong::transform( coord, towgs84fld_->isChecked(),
				    inpcrdsysselfld_->getCoordSystem() );
	survcoord = survinfo_.getCoordSystem()->convertFrom(
			coord, *inpcrdsysselfld_->getCoordSystem() );
    }

    Coord outcrd;
    if ( inpcrdsysselfld_ && outcrdsysselfld_ )
	outcrd = outcrdsysselfld_->getCoordSystem()->convertFrom(
			coord, *inpcrdsysselfld_->getCoordSystem() );

    const BinID outbid( survinfo_.transform(survcoord) );

    xyoutfld_->setValue( outcrd );
    inlcrloutfld_->setValue( outbid );
    if ( lloutfld_ )
	lloutfld_->set( outll );
}


#define mErrRetWithMsg(msg) \
{ \
    uiMSG().error( msg ); \
    return; \
} \

//FILE CONVERSION GROUP
//Commented lines have to be implemented.
uiFileConvGroup::uiFileConvGroup(uiParent* p, const SurveyInfo& si)
    :uiDlgGroup(p, tr("File Conversion"))
    , fd_(uiConvPosAscIO::getDesc())
    , survinfo_(si)
    , ostream_(0)
{
    uiGroup* topgrp = new uiGroup( this, "Top Group" );

    uiFileInput::Setup fssu;
    fssu.withexamine( true ).examstyle( File::Table );
    inpfilefld_ = new uiFileInput( topgrp,
			    uiStrings::phrInput(uiStrings::sFile()), fssu );

    dataselfld_ = new uiTableImpDataSel( topgrp, *fd_,
				    mODHelpKey(mTableImpDataSelwellsHelpID) );
    dataselfld_->attach( alignedBelow, inpfilefld_ );
    mAttachCB(dataselfld_->descChanged, uiFileConvGroup::inpFileSpecChg);

    uiSeparator* sep = new uiSeparator( this, "Inp-Out Sep" );
    sep->attach( stretchedBelow, topgrp );

    uiGroup* botgrp = new uiGroup( this, "BottomGroup" );
    botgrp->attach( alignedBelow, topgrp );
    botgrp->attach( ensureBelow, sep );

    insertpos_ = new uiGenInput( botgrp, tr("Insert at"),
			    BoolInpSpec(true, tr("Beginning"), tr("End")) );

    towgs84fld_ = new uiCheckBox( botgrp, tr("Output to WGS84 CRS") );
    towgs84fld_->setChecked( false );
    towgs84fld_->attach( rightOf, insertpos_ );

    uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Output types"),
					    uiListBox::LblPos::LeftMid );
    su.prefnrlines( uiConvertPos::DataTypeDef().size() + 1 );
    outtypfld_ = new uiListBox( botgrp, su );
    outtypfld_->addItems( uiConvertPos::DataTypeDef().strings() );
    outtypfld_->attach( alignedBelow, insertpos_ );
    mAttachCB(outtypfld_->itemChosen, uiFileConvGroup::outTypChg);

    outcrdsysselfld_ = new Coords::uiCoordSystemSel( botgrp, true, true,
		si.getCoordSystem(), tr("Output Coordinate System") );
    outcrdsysselfld_->attach( alignedBelow, outtypfld_ );

    fssu.forread( false );
    outfilefld_ = new uiFileInput( botgrp,
			uiStrings::phrOutput(uiStrings::sFile()), fssu );
    outfilefld_->attach( alignedBelow, outcrdsysselfld_ );

    uiSeparator* lowersep = new uiSeparator( this, "LoweSep" );
    lowersep->attach( stretchedBelow, botgrp );

    convertbut_ = new uiPushButton( this, uiStrings::sConvert(), true );
    convertbut_->attach( centeredBelow, botgrp );
    convertbut_->attach( ensureBelow, lowersep );
    mAttachCB(convertbut_->activated, uiFileConvGroup::convButPushCB);
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
			    .getEnumForIndex(selidxs[0]) == DataSelType::IC )
	outcrdsysselfld_->display( false );
    else
	outcrdsysselfld_->display( true );

}


void uiFileConvGroup::llFormatTypChg( CallBacker* )
{
    const int selval = lltypfld_->getIntValue();
    lltypfld_->setToolTip( selval ? sDMSDescStr() : sDecDescStr() );
}


void uiFileConvGroup::inpFileSpecChg( CallBacker* )
{
    od_istream istream( "" );
    uiConvPosAscIO aio( *fd_, istream );

    uiStringSet outnms;
    outtypfld_->setEmpty();
    outdisptypidxs_.setEmpty();

    for ( int idx = 0; idx < uiConvertPos::DataTypeDef().size(); idx++ )
    {
	if ( idx == DataSelType::IC &&
				    aio.getConvFromTyp() == DataSelType::IC )
	    continue;

	outnms.add( uiConvertPos::DataTypeDef().getUiStringForIndex(idx) );
	outdisptypidxs_ += idx;
    }

    outtypfld_->addItems( outnms );
}


void uiFileConvGroup::convButPushCB( CallBacker* )
{
    const BufferString inpfnm = inpfilefld_->fileName();

    od_istream istream( inpfnm );
    if ( !istream.isOK() )
	mErrRetWithMsg(uiStrings::phrCannotOpenInpFile());

    const BufferString outfnm = outfilefld_->fileName();

    delete ostream_;
    ostream_ = new od_ostream( outfnm );
    if ( !ostream_->isOK() )
    {
	deleteAndZeroPtr( ostream_ );
	mErrRetWithMsg(uiStrings::phrCannotOpenOutpFile());
    }

    uiConvPosAscIO aio( *fd_, istream );
    const uiConvertPos::DataType fromdatatype = aio.getConvFromTyp();
    Coord crd;
    bool convtoxy = false; bool convtoic = false; bool convtoll = false;
    TypeSet<int> idxs;
    outtypfld_->getChosen( idxs );
    for ( int idx = 0; idx < idxs.size(); idx++ )
    {
	const int outidx = outdisptypidxs_[idxs[idx]];
	if ( outidx == DataSelType::XY )
	    convtoxy = true;
	else if ( outidx == DataSelType::LL )
	    convtoll = true;
	else
	    convtoic = true;
    }

    Coord outcrd;
    LatLong outll;
    BinID outbid;
    od_istream* inpstream = new od_istream(inpfnm);

    const bool towgs = towgs84fld_->isChecked();
    while ( aio.getData(crd) )
    {
	const double firstinp = crd.x;
	const double secondinp = crd.y;

	if ( fromdatatype == DataSelType::LL )
	{
	    const LatLong ll( firstinp, secondinp );
	    if ( !ll.isDefined() )
		continue;

	    const Coord coord = LatLong::transform( ll, towgs,
		    				   survinfo_.getCoordSystem() );
	    outcrd = Coords::CoordSystem::convert( coord,
		    			*survinfo_.getCoordSystem(),
					*outcrdsysselfld_->getCoordSystem() );

	    outll = LatLong::transform( coord, towgs,
					outcrdsysselfld_->getCoordSystem() );
	    outbid = survinfo_.transform( coord );
	}
	else if ( fromdatatype == DataSelType::IC )
	{
	    const BinID bid( firstinp, secondinp );
	    if ( bid.isUdf() )
		continue;

	    outcrd = survinfo_.transform( bid );
	    outll = LatLong::transform( outcrd, towgs,
					outcrdsysselfld_->getCoordSystem() );

	    outcrd = outcrdsysselfld_->getCoordSystem()->convertFrom(
					  outcrd, *survinfo_.getCoordSystem() );
	}
	else
	{
	    const Coord coord( firstinp, secondinp );
	    //this coord is once converted from File CRS to survinfo_.CRS

	    outll = LatLong::transform( coord, towgs,
				    outcrdsysselfld_->getCoordSystem() );

	    outcrd = outcrdsysselfld_->getCoordSystem()->convertFrom(
					outcrd, *survinfo_.getCoordSystem() );

	    outbid = survinfo_.transform(coord);
	}

	const bool addcol = true;
	const bool convdatainbeg = insertpos_->getBoolValue();

	BufferString filestr;
	inpstream->getLine(filestr);

	if ( addcol && !convdatainbeg )
	    ostream_->add( filestr ).add( od_tab );

	if ( addcol )
	{
	    if ( convtoxy )
		ostream_->add( outcrd.x ).add( od_tab ).add( outcrd.y )
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

	ostream_->add( "\n" );
    }

    ostream_->close();

    delete inpstream;

    uiMSG().message( tr("File written successfuly") );
}
