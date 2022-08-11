/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman
 Date:		May 2017
________________________________________________________________________

-*/

#include "uicrssystem.h"

#include "od_helpids.h"
#include "od_iostream.h"
#include "oddirs.h"
#include "sorting.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilatlonginp.h"
#include "uilineedit.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uistring.h"
#include "uitextedit.h"
#include "uitoolbutton.h"


using namespace Coords;

static StringView sKeyEPSG()	{ return StringView("EPSG"); }

static AuthorityCode cDefProjID()
{ return AuthorityCode(sKeyEPSG(),32631); }

uiProjectionBasedSystem::uiProjectionBasedSystem( uiParent* p )
    : uiCoordSystem( p,sFactoryDisplayName() )
{
    createGUI();
    fetchList();
    fillList();

}


uiProjectionBasedSystem::uiProjectionBasedSystem( uiParent* p, bool orthogonal )
    : uiCoordSystem( p,sFactoryDisplayName() )
{
    orthogonal_ = orthogonal;
    createGUI();
    fetchList();
    fillList();
}


uiProjectionBasedSystem::~uiProjectionBasedSystem()
{
    delete convdlg_;
}


void uiProjectionBasedSystem::createGUI()
{
    uiListBox::Setup su( OD::ChooseOnlyOne, tr("Select projection") );
    projselfld_ = new uiListBox( this, su, "ProjectionList" );
    projselfld_->setHSzPol( uiObject::WideVar );
    projselfld_->setNrLines( 10 );
    projselfld_->selectionChanged.notify(
				mCB(this,uiProjectionBasedSystem,selChgCB) );

    uiStringSet filteroptions;
    filteroptions.add( tr("Area of use") ).add( tr("Authority code") )
		 .add( tr("Authority name") ).add( uiStrings::sName() )
		 .add( tr("Projection method") );
    filtersel_ = new uiLabeledComboBox( this, filteroptions, tr("Filter by") );
    filtersel_->box()->setCurrentItem( 3 );
    filtersel_->attach( alignedAbove, projselfld_ );
    filtersel_->setStretch( 0, 0 );

    searchfld_ = new uiLineEdit( this, "Search" );
    searchfld_->setPlaceholderText( tr("ID or name") );
    searchfld_->attach( rightOf, filtersel_ );
    searchfld_->returnPressed.notify(
				mCB(this,uiProjectionBasedSystem,searchCB) );

    uiButton* searchbut = new uiToolButton( this, "search", tr("Search"),
				mCB(this,uiProjectionBasedSystem,searchCB) );
    searchbut->attach( rightOf, searchfld_ );

    uiToolButton* infobut = new uiToolButton( projselfld_, "info",
		tr("View details"), mCB(this,uiProjectionBasedSystem,infoCB) );
    infobut->attach( rightTo, projselfld_->box() );
    infobut->attach( rightBorder );


    uiToolButton* tb = new uiToolButton( projselfld_, "xy2ll",
				tr("Transform XY from/to lat long"),
				mCB(this,uiProjectionBasedSystem,convCB) );
    tb->attach( alignedBelow, infobut );

    setHAlignObj( projselfld_ );
}


bool uiProjectionBasedSystem::initFields( const Coords::CoordSystem* sys )
{
    mDynamicCastGet( const Coords::ProjectionBasedSystem*,projsys,sys)
    if ( !projsys || !projsys->isOK() )
	return false;

    Coords::AuthorityCode pid = orthogonal_ ?
				projsys->getProjection()->authCode() :
				projsys->getProjection()->getGeodeticAuthCode();
    curselidx_ = crsinfolist_->indexOf( pid );
    setCurrent();
    return true;
}


void uiProjectionBasedSystem::searchCB( CallBacker* )
{
    BufferString str = searchfld_->text();
    if ( str.isEmpty() ) // No Filter, display all.
    {
	dispidxs_.erase();
	dispidxs_.setCapacity( crsinfolist_->size(), true );
	for ( int idx=0; idx<crsinfolist_->size(); idx++ )
	    dispidxs_.add( idx );

	fillList();
	return;
    }

    if ( str.size() < 2 ) return; // Not enough to search for.

    BufferString gestr = str;
    if ( !str.find('*') )
    {
	gestr = "*";
	gestr.add( str ).add( '*' );
    }

    MouseCursorChanger mcch( MouseCursor::Wait );
    dispidxs_.erase();
    const int filtertype = filtersel_->box()->currentItem();
    for ( int idx=0; idx<crsinfolist_->size(); idx++ )
    {
	BufferString crsattr;
	switch ( filtertype )
	{
	    case 0: crsattr = crsinfolist_->areaName( idx ); break;
	    case 1: crsattr = crsinfolist_->authCode( idx ); break;
	    case 2: crsattr = crsinfolist_->authName( idx ); break;
	    case 3: crsattr = crsinfolist_->name( idx ); break;
	    case 4: crsattr = crsinfolist_->projMethod( idx ); break;
	}

	if ( crsattr.matches(gestr,CaseInsensitive) )
	    dispidxs_.add( idx );
    }

    fillList();
}


void uiProjectionBasedSystem::fetchList()
{
    crsinfolist_ = getCRSInfoList( orthogonal_ );
    curselidx_ = crsinfolist_->indexOf( cDefProjID() );
    dispidxs_.setCapacity( crsinfolist_->size(), true ); \
    for ( int idx=0; idx<crsinfolist_->size(); idx++ ) \
	dispidxs_.add( idx );
}


void uiProjectionBasedSystem::fillList()
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    projselfld_->setEmpty();
    uiStringSet itemstodisplay;
    for ( int idx=0; idx<dispidxs_.size(); idx++ )
	itemstodisplay.add( crsinfolist_->getDispString(dispidxs_[idx]) );

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

    const AuthorityCode pid( crsinfolist_->authName(dispidxs_[selidx]),
			     crsinfolist_->authCode(dispidxs_[selidx]) );
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


void uiProjectionBasedSystem::infoCB( CallBacker* )
{
    const int selidx = projselfld_->currentItem();
    if ( !dispidxs_.validIdx(selidx) )
	return;

    const uiString nm = crsinfolist_->getDispString( dispidxs_[selidx] );
    uiDialog infodlg( this, uiDialog::Setup(nm,mNoDlgTitle,mNoHelpKey) );
    infodlg.setCtrlStyle( uiDialog::CloseOnly );
    auto* txtfld = new uiTextEdit( &infodlg );
    txtfld->setText( crsinfolist_->getDescString(dispidxs_[selidx]) );
    txtfld->setPrefHeightInChar( 10 );
    infodlg.go();
}


void uiProjectionBasedSystem::convCB( CallBacker* )
{
    if ( !acceptOK() )
	return;

    if ( !convdlg_ )
    {
	const TrcKeyZSampling survtkzs = si_->sampling( true );
	const Coord centerpos =
		si_->transform( survtkzs.hsamp_.center().binID() );
	convdlg_ = new uiConvertGeographicPos( this, outputsystem_.ptr(),
						centerpos );
    }

    convdlg_->go();
}


uiCoordSystem* uiGeodeticCoordSystem::getCRSGeodeticFld( uiParent* p )
{
    return new uiGeodeticCoordSystem( p );
}


static BufferString lastinpfile;
static BufferString lastoutfile;

uiConvertGeographicPos::uiConvertGeographicPos( uiParent* p,
				ConstRefMan<Coords::CoordSystem> coordsystem,
				const Coord& initialpos )
	: uiDialog(p, uiDialog::Setup(tr("Convert Geographical Positions"),
		   mNoDlgTitle, mODHelpKey(mConvertPosHelpID)).modal(false))
	, coordsystem_(coordsystem)
{
    dirfld_ = new uiGenInput( this, tr("Direction"),
		  BoolInpSpec(true,tr("X/Y to Lng/Lat"),tr("Lng/Lat to X/Y")) );
    dirfld_->valuechanged.notify( mCB(this,uiConvertGeographicPos,selChg) );

    towgs84fld_ = new uiCheckBox( this, tr("Output to WGS84 CRS") );
    towgs84fld_->setChecked( false );
    towgs84fld_->attach( alignedBelow, dirfld_ );

    fromwgs84fld_ = new uiCheckBox( this, tr("Input is WGS84 CRS") );
    fromwgs84fld_->setChecked( false );
    fromwgs84fld_->attach( alignedBelow, dirfld_ );

    ismanfld_ = new uiGenInput( this, tr("Conversion"),
		   BoolInpSpec(true,uiStrings::sManual(),uiStrings::sFile()) );
    ismanfld_->valuechanged.notify( mCB(this,uiConvertGeographicPos,selChg) );
    ismanfld_->attach( alignedBelow, towgs84fld_ );

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
    uiLabel* lbl = new uiLabel( filegrp_, tr("Please ensure the format of the "
	"file is X-Y or Long-Lat") );
    lbl->attach( alignedAbove, inpfilefld_ );

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
    postFinalize().notify( mCB(this,uiConvertGeographicPos,finalizeCB) );
}


void uiConvertGeographicPos::finalizeCB( CallBacker* )
{
    selChg( nullptr );
    applyCB( nullptr );
}


void uiConvertGeographicPos::setCoordSystem(
				ConstRefMan<Coords::CoordSystem> newsys )
{
    coordsystem_ = newsys;
    applyCB( nullptr );
}


void uiConvertGeographicPos::selChg( CallBacker* )
{
    const bool tolatlong = dirfld_->getBoolValue();

    towgs84fld_->display( tolatlong );
    fromwgs84fld_->display( !tolatlong );

    const bool isman = ismanfld_->getBoolValue();
    mangrp_->display( isman );
    filegrp_->display( !isman );
}


void uiConvertGeographicPos::applyCB( CallBacker* cb )
{
    if ( !coordsystem_ || !coordsystem_->geographicTransformOK() )
	return;

    const bool isman = ismanfld_->getBoolValue();
    if ( isman )
	convPos();
    else if ( !isman && cb )
	convFile();
}


void uiConvertGeographicPos::convPos()
{
    const bool tolatlong = dirfld_->getBoolValue();
    const bool wgs84 = tolatlong ? towgs84fld_->isChecked()
				 : fromwgs84fld_->isChecked();
    if ( tolatlong )
    {
	const Coord inpcoord( xfld_->getDValue(), yfld_->getDValue() );
	if ( inpcoord.isUdf() ) return;
	latlngfld_->set( LatLong::transform(inpcoord,wgs84,coordsystem_) );
    }
    else
    {
	LatLong inputpos;
	latlngfld_->get( inputpos );
	if ( !inputpos.isDefined() ) return;
	const Coord ouputcoord(LatLong::transform(inputpos,wgs84,coordsystem_));
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
    const bool wgs84 = toll ? towgs84fld_->isChecked()
			    : fromwgs84fld_->isChecked();
    double d1, d2;
    Coord coord; LatLong ll;
    int nrvals = 0;
    while ( istream.isOK() )
    {
	mSetUdf(d1); mSetUdf(d2);
	istream >> d1 >> d2;
	if ( mIsUdf(d1) || mIsUdf(d2) )
	    continue;

	BufferString trailingbufs;
	istream.getLine( trailingbufs );

	if ( toll )
	{
	    coord.x = d1;
	    coord.y = d2;
	    if ( !SI().isReasonable(coord) )
		continue;
	    ll = LatLong::transform( coord, wgs84, coordsystem_ );
	    ostream << ll.lng_ << od_tab << ll.lat_;

	    if ( !trailingbufs.isEmpty() )
		ostream << od_tab << trailingbufs;
	}
	else
	{
	    ll.lng_ = d1; ll.lat_ = d2;
	    coord = LatLong::transform( ll, wgs84, coordsystem_ );
	    if ( !SI().isReasonable(coord) )
		continue;

	    ostream << coord.x << od_tab << coord.y;

	    if ( !trailingbufs.isEmpty() )
		ostream << od_tab << trailingbufs;
	}

	nrvals++;

	if ( !ostream.isOK() )
	    break;

	ostream << od_endl;
    }

    if ( nrvals > 0 )
	uiMSG().message( tr("Total number of points converted: %1")
							    .arg(nrvals) );
}
