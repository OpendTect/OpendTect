/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiconvpos.h"

#include "coordsystem.h"
#include "latlong.h"
#include "od_helpids.h"
#include "od_istream.h"
#include "od_ostream.h"
#include "tableascio.h"
#include "tableascio.h"
#include "tabledef.h"
#include "survinfo.h"
#include "uistrings.h"

#include "uibutton.h"
#include "uicoordsystem.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uifileinput.h"
#include "uilatlonginp.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitblimpexpdatasel.h"
#include "uimsg.h"


#define mMaxLineBuf 32000

mDefineEnumUtils(uiConvertPos, DataType, "Conversion Type")
{
    "X/Y coordinate",
    "Inline/Crossline",
    "Latitude/Longitude",
    nullptr
};


template<>
void EnumDefImpl<uiConvertPos::DataType>::init()
{
    uistrings_ += uiConvertPos::sXYStr();
    uistrings_ += uiConvertPos::sICStr();
    uistrings_ += uiConvertPos::sLLStr();
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


uiConvPosAscIO::~uiConvPosAscIO()
{}


Table::FormatDesc* uiConvPosAscIO::getDesc( const SurveyInfo& si )
{
    const bool isprojcrs = si.getCoordSystem()->isProjection();
    Table::FormatDesc* fd = new Table::FormatDesc( "ConvPos" );
    fd->bodyinfos_ +=
	Table::TargetInfo::mkHorPosition( true, true, isprojcrs, isprojcrs,
					  si.getCoordSystem() );
    return fd;
}


uiConvertPos::DataType uiConvPosAscIO::getConvFromTyp()
{
    if ( isXY() )
	return uiConvertPos::XY;
    else if ( isLL() )
	return uiConvertPos::LL;
    else
	return uiConvertPos::IC;
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


bool uiConvPosAscIO::getData( Coord& crd,
			      ConstRefMan<Coords::CoordSystem> outcrs )
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
	crd = getPos( 0, 1, udfval_, false, outcrs );
    else if ( isIC() )
    {
	const BinID bid = getBinID( 0, 1, udfval_ );
	crd.x = bid.inl();
	crd.y = bid.crl();
    }
    else if ( isLL() )
	crd = getPos( 0, 1, udfval_, true, outcrs );

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
    : uiDlgGroup(p, tr("Manual Conversion"))
    , survinfo_(si)
    , datatypedef_(*new EnumDef(uiConvertPos::DataTypeDef()))
{
    const bool isprojcrs = si.getCoordSystem()->isProjection();
    if ( !isprojcrs )
       datatypedef_.remove(
	       uiConvertPos::DataTypeDef().getKey(uiConvertPos::LL) );

    auto* ingrp = new uiGroup( this, "TopGroup" );

    inptypfld_ = new uiGenInput( ingrp, tr("Convert from"),
				 StringListInpSpec(datatypedef_) );
    mAttachCB( inptypfld_->valuechanged, uiManualConvGroup::inputTypChg );
    ingrp->setHAlignObj( inptypfld_ );

    uiObject* attachobj = inptypfld_->attachObj();
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
				   IntInpSpec().setName("Inl"),
				   IntInpSpec().setName("Crl") );
    inlcrlinfld_->setElemSzPol( uiObject::Medium );
    inlcrlinfld_->attach( alignedBelow, inptypfld_ );

    if ( isprojcrs )
    {
	inpllsysselfld_ = new Coords::uiLatLongSystemSel( ingrp,
			tr("Input Lat-Long System"), si.getCoordSystem() );
	inpllsysselfld_->attach( alignedBelow, inptypfld_ );
	attachobj = inpllsysselfld_->attachObj();
    }

    llinfld_ = new uiLatLongInp( ingrp );
    llinfld_->attach( alignedBelow, attachobj );

    auto* midgrp = new uiGroup( this, "MidGroup" );
    midgrp->attach( alignedBelow, ingrp );
    if ( isprojcrs )
    {
	uiSeparator* sep1 = new uiSeparator( this, "Sep1" );
	sep1->setStretch( 2, 0 );
	sep1->attach( stretchedBelow, ingrp );
	midgrp->attach( ensureBelow, sep1 );

	outcrdsysselfld_ = new Coords::uiCoordSystemSel( midgrp, true, true,
	    si.getCoordSystem(), tr("Output Coordinate System") );

	outllsysselfld_ = new Coords::uiLatLongSystemSel( midgrp,
			tr("Output Lat-Long System"), si.getCoordSystem() );
	outllsysselfld_->attach( alignedBelow, outcrdsysselfld_ );
	midgrp->setHAlignObj( outcrdsysselfld_ );
    }

    convertbut_ = new uiPushButton( midgrp, uiStrings::sConvert(), true );
    if ( outllsysselfld_ )
	convertbut_->attach( centeredBelow, outllsysselfld_ );
    else
	convertbut_->attach( hCentered );

    mAttachCB(convertbut_->activated,uiManualConvGroup::convButPushCB);

    uiSeparator* sep2 = new uiSeparator( this, "Sep2" );
    sep2->setStretch( 2, 0 );
    sep2->attach( stretchedBelow, midgrp );

    auto* outgrp = new uiGroup( this, "BottomGroup" );
    outgrp->attach( alignedBelow, midgrp );
    outgrp->attach( ensureBelow, sep2 );
    outgrp->setVSpacing( 10 );

    xyoutfld_ = new uiGenInput( outgrp, uiConvertPos::sXYStr(),
					DoubleInpSpec(), DoubleInpSpec() );
    xyoutfld_->setElemSzPol( uiObject::Medium );
    xyoutfld_->setReadOnly( true );
    outgrp->setHAlignObj( xyoutfld_ );

    inlcrloutfld_ = new uiGenInput( outgrp, uiConvertPos::sICStr(),
				    IntInpSpec(), IntInpSpec() );
    inlcrloutfld_->setElemSzPol( uiObject::Medium );
    inlcrloutfld_->setReadOnly( true );
    inlcrloutfld_->attach( alignedBelow, xyoutfld_ );

    if ( isprojcrs )
    {
	lloutfld_ = new uiLatLongInp( outgrp );
	lloutfld_->setReadOnly( true );
	lloutfld_->attach( alignedBelow, inlcrloutfld_ );
    }

    inputTypChg( nullptr );
}


uiManualConvGroup::~uiManualConvGroup()
{
    detachAllNotifiers();
    delete &datatypedef_;
}


void uiManualConvGroup::inputTypChg( CallBacker* )
{
    const int selval = inptypfld_->getIntValue();
    xyinfld_->display( selval==uiConvertPos::XY );
    inlcrlinfld_->display( selval==uiConvertPos::IC );
    llinfld_->display( selval==uiConvertPos::LL );
    if ( inpcrdsysselfld_ )
	inpcrdsysselfld_->display( selval==uiConvertPos::XY );
    if ( inpllsysselfld_ )
	inpllsysselfld_->display( selval==uiConvertPos::LL );
}


void uiManualConvGroup::convButPushCB( CallBacker* )
{
    const int selval = inptypfld_->getIntValue();
    if ( selval == uiConvertPos::LL )
	convFromLL();
    else if ( selval == uiConvertPos::IC )
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
}

void uiManualConvGroup::convFromLL()
{
    if ( !inpllsysselfld_ )
	mErrRet( tr("No CRS has been set for this survey.\n"
		    "Please provide a CRS in your survey settings.") )

    if ( !inpllsysselfld_->getCoordSystem() )
	mErrRet( tr("Please selet an input Lat-Long system") )

    LatLong ll;
    llinfld_->get( ll );
    if ( !ll.isDefined() )
	mErrRet( tr("Lat-Long value not defined") )

    const Coord llascrd( ll.lng_, ll.lat_ );
    const Coord incrd = survinfo_.getCoordSystem()->convertFrom( llascrd,
					*inpllsysselfld_->getCoordSystem() );
    Coord outcrd;
    if ( outcrdsysselfld_ && outcrdsysselfld_->getCoordSystem() )
	outcrd = outcrdsysselfld_->getCoordSystem()->convertFrom( llascrd,
					*inpllsysselfld_->getCoordSystem() );

    xyoutfld_->setValue( outcrd );
    const BinID outbid( survinfo_.transform(incrd) );
    inlcrloutfld_->setValue( outbid );

    if ( outllsysselfld_ && outllsysselfld_->getCoordSystem() )
    {
	const Coord outllascrd =
	    outllsysselfld_->getCoordSystem()->convertFrom( incrd,
						*survinfo_.getCoordSystem() );
	lloutfld_->set( LatLong(outllascrd.y,outllascrd.x) );
    }
}


void uiManualConvGroup::convFromIC()
{
    const BinID bid = inlcrlinfld_->getBinID();
    if ( bid.isUdf() )
	mErrRet( sErrMsg() )

    const Coord crd( survinfo_.transform(bid) );
    Coord outcrd = crd;
    if ( outcrdsysselfld_ )
	outcrd = outcrdsysselfld_->getCoordSystem()->convertFrom(
				crd, *survinfo_.getCoordSystem() );

    xyoutfld_->setValue( outcrd );
    inlcrloutfld_->setValue( bid );
    if ( outllsysselfld_ && outllsysselfld_->getCoordSystem() )
    {
	const Coord outllascrd =
	    outllsysselfld_->getCoordSystem()->convertFrom( crd,
						*survinfo_.getCoordSystem() );
	lloutfld_->set( LatLong(outllascrd.y,outllascrd.x) );
    }
}


void uiManualConvGroup::convFromXY()
{
    const Coord incrd = xyinfld_->getCoord();
    if ( incrd.isUdf() )
	mErrRet( sErrMsg() )

    Coord survcrd = incrd;
    Coord outcrd = incrd;
    if ( inpcrdsysselfld_ && inpcrdsysselfld_->getCoordSystem() )
    {
	survcrd = survinfo_.getCoordSystem()->convertFrom(
			incrd, *inpcrdsysselfld_->getCoordSystem() );
	if ( outcrdsysselfld_ && outcrdsysselfld_->getCoordSystem() )
	    outcrd = outcrdsysselfld_->getCoordSystem()->convertFrom(
			incrd, *inpcrdsysselfld_->getCoordSystem() );
    }

    xyoutfld_->setValue( outcrd );
    const BinID outbid( survinfo_.transform( survcrd) );
    inlcrloutfld_->setValue( outbid );
    if ( outllsysselfld_ && outllsysselfld_->getCoordSystem() )
    {
	const Coord outllascrd =
	    outllsysselfld_->getCoordSystem()->convertFrom( survcrd,
						*survinfo_.getCoordSystem() );
	lloutfld_->set( LatLong(outllascrd.y,outllascrd.x) );
    }
}


#define mErrRetWithMsg(msg) \
{ \
    uiMSG().error( msg ); \
    return; \
} \

//FILE CONVERSION GROUP
uiFileConvGroup::uiFileConvGroup(uiParent* p, const SurveyInfo& si)
    :uiDlgGroup(p, tr("File Conversion"))
    , fd_(uiConvPosAscIO::getDesc(si))
    , survinfo_(si)
    , datatypedef_(*new EnumDef(uiConvertPos::DataTypeDef()))
{
    const bool isprojcrs = si.getCoordSystem()->isProjection();
    if ( !isprojcrs )
       datatypedef_.remove(
	       uiConvertPos::DataTypeDef().getKey(uiConvertPos::LL) );

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

    uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Output types"),
			 uiListBox::LblPos::LeftMid );
    su.prefnrlines( datatypedef_.size() + 1 );
    outtypfld_ = new uiListBox( botgrp, su );
    outtypfld_->addItems( datatypedef_.strings() );
    outtypfld_->attach( alignedBelow, insertpos_ );
    mAttachCB(outtypfld_->itemChosen, uiFileConvGroup::outTypChg);

    if ( isprojcrs )
    {
	outcrdsysselfld_ = new Coords::uiCoordSystemSel( botgrp, true, true,
		    si.getCoordSystem(), tr("Output Coordinate System") );
	outcrdsysselfld_->attach( alignedBelow, outtypfld_ );

	outllsysselfld_ = new Coords::uiLatLongSystemSel( botgrp,
			    tr("Output Lat-Long System"), si.getCoordSystem() );
	outllsysselfld_->attach( alignedBelow, outcrdsysselfld_ );
    }

    fssu.forread( false );
    outfilefld_ = new uiFileInput( botgrp,
			uiStrings::phrOutput(uiStrings::sFile()), fssu );
    if ( outllsysselfld_ )
	outfilefld_->attach( alignedBelow, outllsysselfld_ );
    else
	outfilefld_->attach( alignedBelow, outtypfld_ );

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
    delete &datatypedef_;
}


void uiFileConvGroup::outTypChg( CallBacker* )
{
    TypeSet<int> selidxs;
    outtypfld_->getChosen( selidxs );
    const bool hasxy = selidxs.isPresent( 0 );
    const bool hasll = selidxs.isPresent( 2 );
    outcrdsysselfld_->display( hasxy );
    outllsysselfld_->display( hasll );
}


void uiFileConvGroup::inpFileSpecChg( CallBacker* )
{
    od_istream istream( "" );
    uiConvPosAscIO aio( *fd_, istream );

    uiStringSet outnms;
    outtypfld_->setEmpty();
    outdisptypidxs_.setEmpty();

    for ( int idx=0; idx<datatypedef_.size(); idx++ )
    {
	if ( idx == datatypedef_.indexOf(uiConvertPos::IC) &&
		aio.getConvFromTyp() == uiConvertPos::IC )
	    continue;

	outnms.add( datatypedef_.getUiStringForIndex(idx) );
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
	deleteAndNullPtr( ostream_ );
	mErrRetWithMsg(uiStrings::phrCannotOpenOutpFile());
    }

    uiConvPosAscIO aio( *fd_, istream );
    const uiConvertPos::DataType fromdatatype = aio.getConvFromTyp();
    Coord crd;
    bool convtoxy = false; bool convtoic = false; bool convtoll = false;
    TypeSet<int> idxs;
    outtypfld_->getChosen( idxs );
    for ( int idx=0; idx<idxs.size(); idx++ )
    {
	const int outidx = outdisptypidxs_[idxs[idx]];
	if ( outidx == uiConvertPos::XY )
	    convtoxy = true;
	else if ( outidx == uiConvertPos::LL )
	    convtoll = true;
	else
	    convtoic = true;
    }

    bool outcrsissurv = true, outcrsisll = false;
    ConstRefMan<Coords::CoordSystem> outcrs = survinfo_.getCoordSystem();
    if ( outcrdsysselfld_ && convtoxy )
    {
	outcrs = outcrdsysselfld_->getCoordSystem();
	outcrsissurv = false;
    }
    else if ( outllsysselfld_ && convtoll )
    {
	outcrs = outllsysselfld_->getCoordSystem();
	outcrsissurv = false;
	outcrsisll = true;
    }

    Coord outcrd;
    Coord outll;
    BinID outbid;
    od_istream* inpstream = new od_istream(inpfnm);
    for ( int lidx=0; lidx<aio.desc().nrHdrLines(); lidx++ )
    {
	BufferString linestr;
	inpstream->getLine( linestr );
	ostream_->add( linestr ).addNewLine();
    }

    while ( aio.getData(crd,outcrs) )
    {
	const double firstinp = crd.x;
	const double secondinp = crd.y;

	if ( fromdatatype == uiConvertPos::IC )
	{
	    const BinID bid( firstinp, secondinp );
	    if ( bid.isUdf() )
		continue;

	    outcrd = survinfo_.transform( bid );
	    if ( outllsysselfld_ && convtoll )
		outll = outllsysselfld_->getCoordSystem()->convertFrom( outcrd,
						*survinfo_.getCoordSystem() );

	    if ( outcrdsysselfld_ && convtoxy )
		outcrd = outcrdsysselfld_->getCoordSystem()->convertFrom(
					  outcrd, *survinfo_.getCoordSystem() );
	}
	else
	{
	    outcrd.setXY( firstinp, secondinp );
	    // this coord has been converted from File CRS to outcrs

	    if ( !outcrsissurv && convtoic )
	    {
		const Coord survcrd =
		    survinfo_.getCoordSystem()->convertFrom( outcrd,
			    outcrsisll ? *outllsysselfld_->getCoordSystem() :
					 *outcrdsysselfld_->getCoordSystem() );
		outbid = survinfo_.transform( survcrd );
	    }

	    if ( outllsysselfld_ && convtoll )
		outll = outcrsisll ? outcrd :
		    outllsysselfld_->getCoordSystem()->convertFrom( outcrd,
			outcrsissurv ? *survinfo_.getCoordSystem() :
					*outcrdsysselfld_->getCoordSystem() );
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
		ostream_->add( outll.y ).add( od_tab ).add( outll.x )
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
    uiMSG().message( tr("File written successfully") );
}
