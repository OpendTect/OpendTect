/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman
 Date:		May 2017
________________________________________________________________________

-*/

#include "uicrssystem.h"

#include "oddirs.h"
#include "od_iostream.h"
#include "sorting.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uilatlonginp.h"
#include "uilistbox.h"
#include "uilineedit.h"
#include "uimsg.h"
#include "uistring.h"
#include "uitoolbutton.h"
#include "od_helpids.h"

using namespace Coords;


static const int cDefProjID = 32631;

uiProjectionBasedSystem::uiProjectionBasedSystem( uiParent* p )
    : uiPositionSystem( p,sFactoryDisplayName() )
    , curselidx_(-1)
    , convdlg_(0)
{
    uiListBox::Setup su( OD::ChooseOnlyOne, tr("Select projection") );
    projselfld_ = new uiListBox( this, su, "ProjectionList" );
    projselfld_->setFieldWidth( 30 );
    projselfld_->setNrLines( 10 );
    projselfld_->selectionChanged.notify(
	    			mCB(this,uiProjectionBasedSystem,selChgCB) );

    uiButton* searchbut = new uiToolButton( this, "search", tr("Search"),
				mCB(this,uiProjectionBasedSystem,searchCB) );
    searchbut->attach( rightAlignedAbove, projselfld_ );

    searchfld_ = new uiLineEdit( this, "Search" );
    searchfld_->setPlaceholderText( tr("ID or name") );
    searchfld_->attach( leftOf, searchbut );
    searchfld_->editingFinished.notify(
				mCB(this,uiProjectionBasedSystem,searchCB) );

    uiToolButton* tb = new uiToolButton( projselfld_, "xy2ll",
			tr("Transform XY from/to lat long"),
			mCB(this,uiProjectionBasedSystem,convCB) );
    tb->attach( rightTo, projselfld_->box() );
    tb->attach( rightBorder );

    setHAlignObj( searchfld_ );
    fetchList();
    fillList();
}


uiProjectionBasedSystem::~uiProjectionBasedSystem()
{ delete convdlg_; }

bool uiProjectionBasedSystem::initFields( const Coords::PositionSystem* sys )
{
    mDynamicCastGet( const Coords::ProjectionBasedSystem*, from, sys );
    if ( !from || !from->isOK() )
	return false;

    Coords::ProjectionID pid = from->getProjection()->id();
    curselidx_ = ids_.indexOf( pid );
    setCurrent();
    return true;
}


void uiProjectionBasedSystem::searchCB( CallBacker* )
{
    const BufferString str = searchfld_->text();
    if ( str.isEmpty() ) // No Filter, display all.
    {
	dispidxs_.erase();
	dispidxs_.setCapacity( ids_.size(), true );
	for ( int idx=0; idx<ids_.size(); idx++ )
	    dispidxs_.add( idx );

	fillList();
	return;
    }

    if ( str.size() < 3 ) return; // Not enough to search for.

    MouseCursorChanger mcch( MouseCursor::Wait );
    if ( str.isNumber(true) ) // Search for ID
    {
	Coords::ProjectionID searchid = str.toInt();
	const int selidx = ids_.indexOf( searchid );
	if ( selidx < 0 )
	{
	    uiMSG().message( tr("Projection ID %1 was not found")
				.arg(searchid) );
	    return;
	}

	dispidxs_.erase();
	dispidxs_ += selidx;
    }
    else // Search for Name
    {
	dispidxs_.erase();
	BufferString gestr = str;
	if ( !str.find('*') )
	{ gestr = '*'; gestr += str; gestr += '*'; }

	for ( int idx=0; idx<names_.size(); idx++ )
	{
	    if ( names_.get(idx).matches(gestr,CaseInsensitive) )
		dispidxs_.add( idx );
	}
    }

    fillList();
}


void uiProjectionBasedSystem::fetchList()
{
    Projection::getAll( ids_, names_, true );
    curselidx_ = ids_.indexOf( cDefProjID );
    dispidxs_.setCapacity( ids_.size(), true ); \
    for ( int idx=0; idx<ids_.size(); idx++ ) \
	dispidxs_.add( idx );
}


void uiProjectionBasedSystem::fillList()
{
    projselfld_->setEmpty();
    uiStringSet itemstodisplay;
    for ( int idx=0; idx<dispidxs_.size(); idx++ )
    {
	const int index = dispidxs_[idx];
	uiString itmtxt = toUiString("[%1] %2" ).arg(ids_[index])
				.arg(names_.get(index));
	itemstodisplay.add( itmtxt );
    }

    projselfld_->addItems( itemstodisplay );
    setCurrent();
}


void uiProjectionBasedSystem::setCurrent()
{
    const int selidx = curselidx_ < 0 ? -1 : dispidxs_.indexOf( curselidx_ );
    projselfld_->setCurrentItem( selidx );
}


bool uiProjectionBasedSystem::acceptOK()
{
    const int selidx = projselfld_->currentItem();
    if ( !dispidxs_.validIdx(selidx) )
	return false;

    const ProjectionID pid = ids_[dispidxs_[selidx]];
    RefMan<ProjectionBasedSystem> res = new ProjectionBasedSystem;
    res->setProjection( pid );
    outputsystem_ = res;
    return true;
}


void uiProjectionBasedSystem::selChgCB( CallBacker* )
{
    if ( !convdlg_ || !acceptOK() )
	return;

    convdlg_->setCoordSystem( outputsystem_.ptr() );
}


void uiProjectionBasedSystem::convCB( CallBacker* )
{
    if ( !acceptOK() )
	return;

    if ( !convdlg_ )
    {
	const TrcKeyZSampling survtkzs = si_->sampling( true );
	const Coord centerpos = survtkzs.hsamp_.center().getCoord();
	convdlg_ = new uiConvertGeographicPos( this, outputsystem_.ptr(),
						centerpos );
    }

    convdlg_->go();
}


static BufferString lastinpfile;
static BufferString lastoutfile;

uiConvertGeographicPos::uiConvertGeographicPos( uiParent* p,
				ConstRefMan<Coords::PositionSystem> coordsystem,
				const Coord& initialpos )
	: uiDialog(p, uiDialog::Setup(tr("Convert Geographical Positions"),
		   mNoDlgTitle, mODHelpKey(mConvertPosHelpID)).modal(false))
	, coordsystem_(coordsystem)
{
    dirfld_ = new uiGenInput( this, tr("Direction"),
	          BoolInpSpec(true,tr("X/Y to Lat/Lng"),tr("Lat/Lng to X/Y")) );

    ismanfld_ = new uiGenInput( this, tr("Conversion"),
	           BoolInpSpec(true,uiStrings::sManual(),uiStrings::sFile()) );
    ismanfld_->valuechanged.notify( mCB(this,uiConvertGeographicPos,selChg) );
    ismanfld_->attach( alignedBelow, dirfld_ );

    mangrp_ = new uiGroup( this, "Manual group" );
    uiGroup* xygrp = new uiGroup( mangrp_, "XY group" );
    xfld_ = new uiGenInput( xygrp, tr("X-coordinate"),
			   DoubleInpSpec().setName("X-field") );
    xfld_->setElemSzPol( uiObject::Medium );
    xfld_->setValue( initialpos.x );
    yfld_ = new uiGenInput( xygrp, tr("Y-coordinate"),
			   DoubleInpSpec().setName("Y-field") );
    yfld_->setElemSzPol( uiObject::Medium );
    yfld_->setValue( initialpos.y );
    yfld_->attach( alignedBelow, xfld_ );
    xygrp->setHAlignObj( xfld_ );

    latlngfld_ = new uiLatLongInp( mangrp_ );
    latlngfld_->attach( alignedBelow, xygrp );

    mangrp_->attach( alignedBelow, ismanfld_ );
    mangrp_->setHAlignObj( xygrp );

    filegrp_ = new uiGroup( this, "File group" );
    uiFileInput::Setup fipsetup( lastinpfile );
    fipsetup.forread(true).withexamine(true)
	    .examstyle(File::Table).defseldir(GetDataDir());
    inpfilefld_ = new uiFileInput( filegrp_, uiStrings::phrInput(
					   uiStrings::sFile()), fipsetup );

    fipsetup.fnm = lastoutfile;
    fipsetup.forread(false).withexamine(false);
    outfilefld_ = new uiFileInput( filegrp_, uiStrings::phrOutput(
					   uiStrings::sFile()), fipsetup );
    outfilefld_->attach( alignedBelow, inpfilefld_ );
    filegrp_->setHAlignObj( inpfilefld_ );
    filegrp_->attach( alignedBelow, ismanfld_ );

    uiPushButton* convbut = new uiPushButton( this, tr("Convert"),
	    		mCB(this,uiConvertGeographicPos,applyCB), true );
    convbut->attach( centeredBelow, mangrp_ );

    setCtrlStyle( CloseOnly );
    postFinalise().notify( mCB(this,uiConvertGeographicPos,finaliseCB) );
}


void uiConvertGeographicPos::finaliseCB( CallBacker* )
{
    selChg(0);
    convPos();
}


void uiConvertGeographicPos::setCoordSystem(
				ConstRefMan<Coords::PositionSystem> newsys )
{
    coordsystem_ = newsys;
    convPos();
}


void uiConvertGeographicPos::selChg( CallBacker* )
{
    const bool isman = ismanfld_->getBoolValue();
    mangrp_->display( isman );
    filegrp_->display( !isman );
}


void uiConvertGeographicPos::applyCB( CallBacker* )
{
    if ( !coordsystem_ || !coordsystem_->geographicTransformOK() )
	return;

    const bool isman = ismanfld_->getBoolValue();
    if ( isman )
	convPos();
    else
	convFile();
}


void uiConvertGeographicPos::convPos()
{
    const bool tolatlong = dirfld_->getBoolValue();
    if ( tolatlong )
    {
	const Coord inpcoord( xfld_->getDValue(), yfld_->getDValue() );
	if ( inpcoord.isUdf() ) return;
	const LatLong outputpos = coordsystem_->toGeographicWGS84( inpcoord );
	latlngfld_->set( outputpos );
    }
    else
    {
	LatLong inputpos;
	latlngfld_->get( inputpos );
	if ( !inputpos.isDefined() ) return;
	const Coord ouputcoord = coordsystem_->fromGeographicWGS84( inputpos );
	xfld_->setValue( ouputcoord.x );
	yfld_->setValue( ouputcoord.y );
    }
}


#define mErrRet(s) { uiMSG().error(s); return; }

void uiConvertGeographicPos::convFile()
{
    const BufferString inpfnm = inpfilefld_->fileName();

    od_istream istream( inpfnm );
    if ( !istream.isOK() )
	mErrRet(tr("Input file is not readable") );

    const BufferString outfnm = outfilefld_->fileName();
    od_ostream ostream( outfnm );
    if ( !ostream.isOK() )
    { mErrRet(uiStrings::sCantOpenOutpFile()); }

    lastinpfile = inpfnm; lastoutfile = outfnm;

    BufferString linebuf; Coord c;
    const bool toll = dirfld_->getBoolValue();
    double d1, d2;
    Coord coord; LatLong ll;
    while ( istream.isOK() )
    {
	mSetUdf(d1); mSetUdf(d2);
	istream >> d1 >> d2;
	if ( mIsUdf(d1) || mIsUdf(d2) )
	    continue;

	if ( toll )
	{
	    coord.x = d1;
	    coord.y = d2;
	    if ( !SI().isReasonable(coord) )
		continue;
	    ll = coordsystem_->toGeographicWGS84( coord );
	    ostream << ll.lat_ << od_tab << ll.lng_;
	}
	else
	{
	    ll.lat_ = d1; ll.lng_ = d2;
	    coord = coordsystem_->fromGeographicWGS84( ll );
	    if ( !SI().isReasonable(coord) )
		continue;
	    ostream << coord.x << od_tab << coord.y;
	}
	if ( !ostream.isOK() )
	    break;
	ostream << od_endl;
    }
}


