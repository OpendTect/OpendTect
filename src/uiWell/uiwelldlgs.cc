/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          October 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiwelldlgs.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicolor.h"
#include "uid2tmodelgrp.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uitable.h"
#include "uitblimpexpdatasel.h"
#include "uiunitsel.h"
#include "uiwellsel.h"

#include "ctxtioobj.h"
#include "file.h"
#include "hiddenparam.h"
#include "iodir.h"
#include "ioman.h"
#include "ioobj.h"
#include "iodirentry.h"
#include "iopar.h"
#include "oddirs.h"
#include "randcolor.h"
#include "od_iostream.h"
#include "survinfo.h"
#include "tabledef.h"
#include "unitofmeasure.h"
#include "veldesc.h"
#include "velocitycalc.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "welltransl.h"
#include "wellreader.h"
#include "wellwriter.h"
#include "welltrack.h"
#include "wellimpasc.h"
#include "od_helpids.h"


static const char* trackcollbls[] = { "X", "Y", "Z", "MD", 0 };
static const int nremptyrows = 5;
static const int cXCol = 0;
static const int cYCol = 1;
static const int cZCol = 2;
static const int cMDTrackCol = 3;

static const char* sKeyMD()		{ return "MD"; }
static const char* sKeyTVD()		{ return "TVD"; }
static const char* sKeyTVDGL()		{ return "TVDGL"; }
static const char* sKeyTVDSD()		{ return "TVDSD"; }
static const char* sKeyTVDSS()		{ return "TVDSS"; }
static const char* sKeyTWT()		{ return "TWT"; }
static const char* sKeyOWT()		{ return "OWT"; }
static const char* sKeyVint()		{ return "Vint"; }


uiString getWinTitle( const uiString& objtyp,
				      const MultiID& wllky, bool& iswr )
{
    iswr = Well::Writer::isFunctional( wllky );
    return toUiString("%1 %2").arg(iswr ? uiStrings::sEdit() :
					  uiStrings::sView()).
			       arg(objtyp);
}

uiString getDlgTitle( const MultiID& wllky )
{
    const BufferString wellnm = IOM().nameOf( wllky );
    return toUiString("%1: %2").arg(uiStrings::sWell()).arg(wellnm);
}

#define mGetDlgSetup(wd,objtyp,hid) \
    uiDialog::Setup( getWinTitle(objtyp,wd.multiID(),writable_), \
		     getDlgTitle(wd.multiID()), mODHelpKey(hid) )
#define mTDName(iscksh) iscksh ? uiWellTrackDlg::sCkShotData() \
			       : uiWellTrackDlg::sTimeDepthModel()
#define mTDOpName(op,iscksh) \
    toUiString("%1 %2").arg(op).arg(mTDName(iscksh))

#define mAddSetBut(fld,cb) \
    if ( writable_ ) \
    { \
	setbut = new uiPushButton( actgrp, uiStrings::sSet(), \
				mCB(this,uiWellTrackDlg,cb), true ); \
	setbut->attach( rightOf, fld ); \
    }

static HiddenParam<uiWellTrackDlg,uiGenInput*> glflds(nullptr);

uiWellTrackDlg::uiWellTrackDlg( uiParent* p, Well::Data& d )
	: uiDialog(p,mGetDlgSetup(d,tr("Well Track"),mWellTrackDlgHelpID))
	, wd_(d)
	, track_(d.track())
	, orgtrack_(new Well::Track(d.track()))
	, fd_( *Well::TrackAscIO::getDesc() )
	, origpos_(mUdf(Coord3))
	, origgl_(d.info().groundelev)
{
    tbl_ = new uiTable( this, uiTable::Setup()
					.rowdesc("Point")
					.rowgrow(true)
					.insertrowallowed(true)
					.removerowallowed(true)
					.selmode(uiTable::Multi)
					.defrowlbl(true)
					.removeselallowed(false),
			"Well Track Table" );
    tbl_->setColumnLabels( trackcollbls );
    tbl_->setNrRows( nremptyrows );
    tbl_->setPrefWidth( 500 );
    tbl_->setPrefHeight( 400 );
    tbl_->setTableReadOnly( !writable_ );
    tbl_->setSelectionBehavior( uiTable::SelectRows );

    uwifld_ = new uiGenInput( this, tr("UWI"), StringInpSpec() );
    uwifld_->setText( wd_.info().uwid );
    uwifld_->attach( leftAlignedBelow, tbl_ );
    if ( !writable_ ) uwifld_->setReadOnly( true );

    zinftfld_ = new uiCheckBox( this, tr("Z in Feet") );
    zinftfld_->setChecked( SI().depthsInFeet() );
    zinftfld_->activated.notify( mCB(this,uiWellTrackDlg,fillTable) );
    zinftfld_->activated.notify( mCB(this,uiWellTrackDlg,fillSetFields) );
    zinftfld_->attach( rightAlignedBelow, tbl_ );

    uiSeparator* sep = new uiSeparator( this, "Sep" );
    sep->attach( stretchedBelow, uwifld_ );

    uiGroup* actgrp = new uiGroup( this, "Action grp" );
    actgrp->attach( ensureBelow, sep );

    uiPushButton* setbut = 0;
    uiString unitstr = SI().getUiXYUnitString();
    wellheadxfld_ = new uiGenInput( actgrp,
				tr("X-Coordinate of well head %1").arg(unitstr),
				DoubleInpSpec(mUdf(double)) );
    mAddSetBut( wellheadxfld_, updateXpos )
    if ( !writable_ ) wellheadxfld_->setReadOnly( true );

    wellheadyfld_ = new uiGenInput( actgrp,
				tr("Y-Coordinate of well head %1").arg(unitstr),
				DoubleInpSpec(mUdf(double)) );
    wellheadyfld_->attach( alignedBelow, wellheadxfld_ );
    mAddSetBut( wellheadyfld_, updateYpos )
    if ( !writable_ ) wellheadyfld_->setReadOnly( true );

    kbelevfld_ = new uiGenInput( actgrp, Well::Info::sKBElev(),
				 FloatInpSpec(mUdf(float)) );
    mAddSetBut( kbelevfld_, updateKbElev )
    kbelevfld_->attach( alignedBelow, wellheadyfld_ );
    if ( !writable_ ) kbelevfld_->setReadOnly( true );

    uiGenInput* glfld = new uiGenInput( actgrp, Well::Info::sGroundElev(),
					FloatInpSpec(mUdf(float)) );
    glfld->attach( alignedBelow, kbelevfld_ );
    if ( !writable_ ) glfld->setReadOnly( true );
    glflds.setParam( this, glfld );

    uiButton* readbut = !writable_ ? 0
		: new uiPushButton( this, uiStrings::sImport(),
				    mCB(this,uiWellTrackDlg,readNew), false );
    if ( readbut )
	readbut->attach( leftAlignedBelow, actgrp );
    uiButton* expbut = new uiPushButton( this, uiStrings::sExport(),
					 mCB(this,uiWellTrackDlg,exportCB),
					 false );
    if ( readbut )
	expbut->attach( rightOf, readbut );
    else
	expbut->attach( leftAlignedBelow, actgrp );
    uiPushButton* updbut = !writable_ ? 0
		: new uiPushButton( this, tr("Update display"),
				   mCB(this,uiWellTrackDlg,updNow), true );
    updbut->attach( rightTo, expbut );
    updbut->attach( rightBorder );

    if ( !track_.isEmpty() )
	origpos_ = track_.pos(0);

    fillTable();
}


uiWellTrackDlg::~uiWellTrackDlg()
{
    delete orgtrack_;
    delete &fd_;

    glflds.removeParam( this );
}


const uiString uiWellTrackDlg::sCkShotData()
{ return tr("Checkshot Data"); }


const uiString uiWellTrackDlg::sTimeDepthModel()
{ return tr("Time-Depth Model"); }


static const UnitOfMeasure* getDisplayUnit( uiCheckBox* zinfeet )
{
    if ( !zinfeet ) return 0;
    return zinfeet->isChecked() ? UoMR().get( "Feet" ) : UoMR().get( "Meter" );
}

#define mDataUom(depth) ( \
	depth ? \
	UnitOfMeasure::surveyDefDepthUnit() : \
	UnitOfMeasure::surveyDefTimeUnit() )

#define mConvertVal(val,toscreen) ( \
	toscreen ? \
 getConvertedValue( mDataUom(true)->userValue(val),mDataUom(true), \
	    getDisplayUnit(zinftfld_) ) : \
 mDataUom(true)->internalValue( \
	     getConvertedValue(val,getDisplayUnit(zinftfld_),mDataUom(true)) ) )

bool uiWellTrackDlg::fillTable( CallBacker* )
{
    RowCol curcell( tbl_->currentCell() );

    const int sz = track_.size();
    int newsz = sz + nremptyrows;
    if ( newsz < 8 ) newsz = 8;
    tbl_->setNrRows( newsz );
    tbl_->clearTable();
    if ( sz < 1 )
	return false;

    for ( int idx=0; idx<sz; idx++ )
    {
	const Coord3& c( track_.pos(idx) );
	setX( idx, c.x );
	setY( idx, c.y );
	setZ( idx, c.z );
	setMD( idx, track_.dah(idx) );
    }

    if ( curcell.row() >= newsz ) curcell.row() = newsz-1;
    tbl_->setCurrentCell( curcell );
    fillSetFields();

    const uiString xyunit = SI().getUiXYUnitString();
    const uiString depthunit = uiStrings::sDistUnitString(
					zinftfld_->isChecked(), true, true );
    tbl_->setColumnLabel( 0, tr("%1 %2").arg(uiStrings::sX()).arg(xyunit) );
    tbl_->setColumnLabel( 1, tr("%1 %2").arg(uiStrings::sY()).arg(xyunit) );
    tbl_->setColumnLabel( 2, tr("%1 %2").arg(sKeyTVDSS()).arg(depthunit) );
    tbl_->setColumnLabel( 3, tr("%1 %2").arg(sKeyMD()).arg(depthunit) );
    return true;
}


void uiWellTrackDlg::fillSetFields( CallBacker* )
{
    NotifyStopper nsx( wellheadxfld_->updateRequested );
    NotifyStopper nsy( wellheadyfld_->updateRequested );
    NotifyStopper nskbelev( kbelevfld_->updateRequested );

    const uiString depthunit = uiStrings::sDistUnitString(
					zinftfld_->isChecked(), true, true );

    kbelevfld_->setTitleText(
		tr("%1 %2 ").arg( Well::Info::sKBElev() ).arg(depthunit) );

    uiGenInput* glfld = glflds.getParam( this );
    glfld->setTitleText(
		tr("%1 %2 ").arg( Well::Info::sGroundElev() ).arg(depthunit) );

    if ( track_.size() > 1 )
	wd_.info().surfacecoord = track_.pos(0);

    Coord wellhead = wd_.info().surfacecoord;
    if ( mIsZero(wellhead.x,0.001) )
	{ wellhead.x = wellhead.y = mUdf(float); }
    wellheadxfld_->setValue( wellhead.x );
    wellheadyfld_->setValue( wellhead.y );

    kbelevfld_->setValue( mConvertVal(track_.getKbElev(),true) );
    glfld->setValue( mConvertVal(wd_.info().groundelev,true) );
}


double uiWellTrackDlg::getX( int row ) const
{
    return tbl_->getdValue( RowCol(row,cXCol) );
}


double uiWellTrackDlg::getY( int row ) const
{
    return tbl_->getdValue( RowCol(row,cYCol) );
}


double uiWellTrackDlg::getZ( int row ) const
{
    return mConvertVal(tbl_->getdValue( RowCol(row,cZCol) ),false);
}


float uiWellTrackDlg::getMD( int row ) const
{
    return mConvertVal(tbl_->getfValue( RowCol(row,cMDTrackCol) ),false);
}


void uiWellTrackDlg::setX( int row, double x )
{
    tbl_->setValue( RowCol(row,cXCol), x, 2 );
}


void uiWellTrackDlg::setY( int row, double y )
{
    tbl_->setValue( RowCol(row,cYCol), y, 2 );
}


void uiWellTrackDlg::setZ( int row, double z )
{
    tbl_->setValue( RowCol(row,cZCol), mConvertVal(z,true), 2 );
}


void uiWellTrackDlg::setMD( int row, float md )
{
    tbl_->setValue( RowCol(row,cMDTrackCol), mConvertVal(md,true), 2 );
}



class uiWellTrackReadDlg : public uiDialog
{ mODTextTranslationClass(uiWellTrackReadDlg);
public:

uiWellTrackReadDlg( uiParent* p, Well::Data& wd )
    : uiDialog(p,uiDialog::Setup(tr("Import New Well Track"),mNoDlgTitle,
				 mODHelpKey(mWellTrackReadDlgHelpID)))
    , wd_(wd)
    , track_(wd.track())
    , fd_(*Well::TrackAscIO::getDesc())
    , dirfd_(*Well::DirectionalAscIO::getDesc())
{
    setOkText( uiStrings::sImport() );

    uiStringSet options;
    options.add( tr("Well Track file (XYZ)") )
	   .add( tr("Directional Well") );
    tracksrcfld_ = new uiGenInput( this, tr("Input type"),
				StringListInpSpec(options) );
    tracksrcfld_->valuechanged.notify( mCB(this,uiWellTrackReadDlg,trckSrcSel));

    wtinfld_ = new uiFileInput( this, uiStrings::phrJoinStrings(
		   uiStrings::sWell(), uiStrings::sTrack(), uiStrings::sFile()),
		   uiFileInput::Setup().withexamine(true) );
    wtinfld_->attach( alignedBelow, tracksrcfld_ );
    wtinfld_->valuechanged.notify( mCB(this,uiWellTrackReadDlg,inputChgd) );

    dataselfld_ = new uiTableImpDataSel( this, fd_,
				mODHelpKey(mWellImportAscDataSelHelpID) );
    dataselfld_->attach( alignedBelow, wtinfld_ );
    dataselfld_->descChanged.notify( mCB(this,uiWellTrackReadDlg,trckFmtChg) );

    dirselfld_ = new uiTableImpDataSel( this, dirfd_,
				mODHelpKey(mWellImportAscDataSelHelpID) );
    dirselfld_->attach( alignedBelow, wtinfld_ );
    dirselfld_->display( false );

    const uiString zunit = UnitOfMeasure::surveyDefDepthUnitAnnot( true, true );
    uiString kblbl = tr( "%1 %2" )
		     .arg(Well::Info::sKBElev()).arg( zunit );
    kbelevfld_ = new uiGenInput( this, kblbl, FloatInpSpec(0) );
    kbelevfld_->setWithCheck();
    kbelevfld_->setChecked( false );
    kbelevfld_->attach( alignedBelow, dataselfld_ );

    uiString tdlbl = tr( "%1 %2" )
		     .arg(Well::Info::sTD()).arg( zunit );
    tdfld_ = new uiGenInput( this, tdlbl, FloatInpSpec() );
    tdfld_->setWithCheck();
    tdfld_->setChecked( false );
    tdfld_->attach( alignedBelow, kbelevfld_ );
}


~uiWellTrackReadDlg()
{
    delete &fd_;
    delete &dirfd_;
}


void trckSrcSel( CallBacker* )
{
    const bool isdir = tracksrcfld_->getIntValue() == 1;
    dataselfld_->display( !isdir );
    dirselfld_->display( isdir );
}


void inputChgd( CallBacker* )
{
    kbelevfld_->setValue( 0 );
    tdfld_->setChecked( false );
    tdfld_->setValue( mUdf(float) );
}


void trckFmtChg( CallBacker* )
{
    const Table::FormatDesc& fd = dataselfld_->desc();
    if ( !fd.isGood() )
	return;

    bool havez = false;
    bool havemd = false;
    for ( int idx=0; idx<fd.bodyinfos_.size(); idx++ )
    {
	const Table::TargetInfo& ti = *fd.bodyinfos_[idx];
	if ( ti.name() == Well::Info::sKeyTVDSS() && ti.selection_.isInFile(0) )
	    havez = true;

	if ( ti.name() == "MD" && ti.selection_.isInFile(0) )
	    havemd = true;
    }

    if ( !havez && !havemd )
    {
	uiMSG().error( tr("The format you defined has neither Z nor MD."
			  "\nYou should define at least one."
			  "\nAs it is now, the track will not load.") );
    }

    kbelevfld_->setChecked( !havez || !havemd );
}


float getKbElev() const
{
    const UnitOfMeasure* zun = UnitOfMeasure::surveyDefDepthUnit();
    if ( !kbelevfld_->isChecked() )
	return mUdf(float);

    if ( mIsUdf(kbelevfld_->getfValue()) )
	return 0;

    if ( zun )
	return zun->internalValue( kbelevfld_->getfValue() );

    return mUdf(float);
}


float getTD() const
{
    const UnitOfMeasure* zun = UnitOfMeasure::surveyDefDepthUnit();
    if ( !tdfld_->isChecked() )
	return mUdf(float);

    if ( zun )
	return zun->internalValue( tdfld_->getfValue() );

    return mUdf(float);
}

#define mErrRet(s) { if ( (s).isSet() ) uiMSG().error(s); return false; }

bool acceptOK( CallBacker* )
{
    track_.setEmpty();
    const BufferString fnm = wtinfld_->fileName();
    if ( File::isEmpty(fnm.buf()) )
	mErrRet( uiStrings::sInvInpFile() );

    const bool isdir = tracksrcfld_->getIntValue() == 1;
    if ( !isdir && !dataselfld_->commit() )
	return false;
    if ( isdir && !dirselfld_->commit() )
	return false;

    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( uiStrings::sCantOpenInpFile() );

    const float kbelev = getKbElev();
    const float td = getTD();
    if ( !isdir )
    {
	Well::TrackAscIO wellascio( fd_, strm );
	if ( !wellascio.getData(wd_,kbelev,td) )
	{
	    uiString msg = tr( "The track file cannot be loaded:\n%1" )
				.arg( wellascio.errMsg() );
	    mErrRet( msg );
	}

	if ( wellascio.warnMsg().isSet() )
	{
	    uiString msg =
		    tr( "The track file loading issued a warning:\n%1" )
		    .arg( wellascio.warnMsg() );
	    uiMSG().warning( msg );
	}
    }
    else if ( isdir )
    {
	Well::DirectionalAscIO dirascio( dirfd_, strm );
	if ( !dirascio.getData(wd_,kbelev) )
	{
	    uiString msg = tr( "The track file cannot be loaded:\n%1" )
				.arg( dirascio.errMsg() );
	    mErrRet( msg );
	}
    }

    return true;
}

    uiGenInput*		tracksrcfld_;
    uiFileInput*	wtinfld_;
    uiGenInput*		kbelevfld_;
    uiGenInput*		tdfld_;
    Well::Data&		wd_;
    Well::Track&	track_;
    Table::FormatDesc&	fd_;
    uiTableImpDataSel*	dataselfld_;
    Table::FormatDesc&	dirfd_;
    uiTableImpDataSel*	dirselfld_;

};


void uiWellTrackDlg::readNew( CallBacker* )
{
    uiWellTrackReadDlg dlg( this, wd_ );
    if ( !dlg.go() ) return;

    tbl_->clearTable();
    if ( !fillTable() )
	return;

    wd_.trackchanged.trigger();
}


bool uiWellTrackDlg::updNow( CallBacker* )
{
    wd_.info().uwid = uwifld_->text();
    wd_.info().groundelev =
	mConvertVal(glflds.getParam(this)->getFValue(),false);

    track_.setEmpty();
    const int nrrows = tbl_->nrRows();

    bool needfill = false;
    for ( int idx=0; idx<nrrows; idx++ )
    {
	if ( rowIsNotSet(idx) )
	    continue;

	if ( rowIsIncomplete(idx) )
	{
	    uiString msg =
		tr("X, Y, Z or MD is not set in row %1.\n"
		   "Please enter a valid value or remove this row.").arg(idx+1);
	    uiMSG().error( msg );
	    return false;
	}

	const double xval = getX( idx );
	const double yval = getY( idx );
	const double zval = getZ(idx);
	float dahval = getMD(idx);
	const Coord3 newc( xval, yval, zval );
	if ( !SI().isReasonable(newc) )
	{
	    uiString msg =
		tr("The coordinate in row %1 seems to be far outside "
		   "the survey.\nDo you want to continue?").arg(idx+1);
	    const bool res = uiMSG().askGoOn( msg );
	    if ( !res ) return false;
	}

	if ( idx > 0 && mIsUdf(dahval) )
	{
	    dahval = track_.dah(idx-1) +
		     mCast(float, track_.pos(idx-1).distTo( newc ) );
	    needfill = true;
	}

	track_.addPoint( newc, mCast(float,newc.z), dahval );
    }

    if ( track_.size() > 1 )
    {
	fillSetFields();
	wd_.trackchanged.trigger();
    }
    else
    {
	uiMSG().error( tr("Please define at least two points.") );
	return false;
    }

    if ( needfill && !fillTable() )
	return false;

    return true;
}


void uiWellTrackDlg::updateXpos( CallBacker* )
{
    updatePos( true );
}


void uiWellTrackDlg::updateYpos( CallBacker* )
{
    updatePos( false );
}


void uiWellTrackDlg::updatePos( bool isx )
{
    uiGenInput* posfld = isx ? wellheadxfld_ : wellheadyfld_;
    const Coord surfacecoord = wd_.info().surfacecoord;
    double surfacepos = isx ? surfacecoord.x : surfacecoord.y;
    if ( mIsUdf(surfacepos) && !track_.isEmpty() )
	surfacepos = isx ? track_.pos(0).x : track_.pos(0).y;

    const double newpos = posfld->getdValue();
    if ( mIsUdf(newpos) )
    {
	uiMSG().error( tr("Please enter a valid coordinate") );
	posfld->setValue( surfacepos );
	return;
    }

    if ( track_.isEmpty() )
    {
	if ( isx )
	{
	    setX( 0, newpos );
	    setX( 1, newpos );
	}
	else
	{
	    setY( 0, newpos );
	    setY( 1, newpos );
	}
	return;
    }

    updNow(0); //ensure the table contains only the clean track
    posfld->setValue( newpos );
    const int icol = isx ? cXCol : cYCol;
    const double shift = newpos - surfacepos;
    for ( int irow=0; irow<tbl_->nrRows(); irow++ )
    {
	const double tblpos = isx ? getX(irow) : getY(irow);
	if ( mIsUdf(tblpos) )
	    continue;

	tbl_->setValue( RowCol(irow,icol), tblpos+shift, 2 );
    }

    updNow(0); //write the new table data back to the track
}


void uiWellTrackDlg::updateKbElev( CallBacker* )
{
    float newkbelev = mConvertVal(kbelevfld_->getfValue(),false);
    float kbelevorig = track_.isEmpty() ? 0.f : track_.getKbElev();
    if ( mIsUdf(newkbelev) )
    {
	uiMSG().error( tr("Please enter a valid %1")
				  .arg(Well::Info::sKBElev()) );
	kbelevfld_->setValue( mConvertVal(kbelevorig,true) );
	return;
    }

    if ( track_.isEmpty() )
    {
	setZ( 0, -1.f * newkbelev );
	setMD( 0, 0.f );
	return;
    }

    updNow(0); //ensure the table contains only the clean track
    kbelevfld_->setValue( mConvertVal(newkbelev,true) );
    const float shift = kbelevorig - newkbelev;
    for ( int irow=0; irow<tbl_->nrRows(); irow++ )
    {
	const double zval = getZ( irow );
	if ( mIsUdf(zval) )
	    continue;

	setZ( irow, mCast(float,zval) + shift );
    }

    updNow(0); //write the new table data back to the track
}


bool uiWellTrackDlg::rowIsIncomplete( int row ) const
{
    return mIsUdf(getX(row)) || mIsUdf(getY(row)) ||
	   mIsUdf(getZ(row)) || mIsUdf(getMD(row));
}


bool uiWellTrackDlg::rowIsNotSet( int row ) const
{
    return mIsUdf(getX(row)) && mIsUdf(getY(row)) &&
	   mIsUdf(getZ(row)) && mIsUdf(getMD(row));
}


bool uiWellTrackDlg::rejectOK( CallBacker* )
{
    track_ = *orgtrack_;
    wd_.info().surfacecoord = origpos_;
    wd_.info().groundelev = origgl_;
    wd_.trackchanged.trigger();
    return true;
}


bool uiWellTrackDlg::acceptOK( CallBacker* )
{
    if ( !writable_ )
	return true;

    if ( !updNow(0) )
	return false;

    const int nrpts = track_.size();
    if ( nrpts < 2 ) return false;
    const int orgnrpts = orgtrack_->size();
    bool dahchg = nrpts != orgnrpts;
    if ( !dahchg )
    {
	for ( int idx=0; idx<nrpts; idx++ )
	{
	    const float dah = track_.dah(idx);
	    const float orgdah = orgtrack_->dah(idx);
	    if ( !mIsEqual(dah,orgdah,0.001) )
		{ dahchg = true; break; }
	}
    }

    if ( dahchg )
    {
	uiString msg = tr("You have changed at least one MD value.\nMarkers%1"
			  " are based on the old MD values.\n"
			  "They may therefore become invalid.\n\nContinue?");
	if ( SI().zIsTime() )
	    msg.arg(tr(", logs, T/D and checkshot models"));
	else
	    msg.arg(tr(" and logs"));
	if ( !uiMSG().askGoOn(msg) )
	    return false;
    }

    return true;
}


void uiWellTrackDlg::exportCB( CallBacker* )
{
    updNow( 0 );
    if ( !track_.size() )
    {
	uiMSG().message( tr("No data available to export") );
	return;
    }

    uiFileDialog fdlg( this, false, 0, 0, tr("File name for export") );
    fdlg.setDefaultExtension( "dat" );
    fdlg.setDirectory( GetDataDir() );
    if ( !fdlg.go() )
	return;

    od_ostream strm( fdlg.fileName() );
    if ( !strm.isOK() )
    {
	uiMSG().error( tr( "Cannot open '%1' for write").arg(fdlg.fileName()) );
	return;
    }

    const bool zinfeet = zinftfld_ ? zinftfld_->isChecked() : false;
    const BufferString depthunit = getDistUnitString( zinfeet, true );

    strm << trackcollbls[0] << SI().getXYUnitString() << od_tab;
    strm << trackcollbls[1] << SI().getXYUnitString() << od_tab;
    strm << trackcollbls[2] << depthunit << od_tab;
    strm << "TVD" << depthunit << od_tab;
    strm << trackcollbls[3] << depthunit << od_newline;

    const float kbdepth = -1.f * track_.getKbElev();
    for ( int idx=0; idx<track_.size(); idx++ )
    {
	const Coord3 coord( track_.pos(idx) );
	strm << coord.x << od_tab;
	strm << coord.y << od_tab;
	strm << mConvertVal(coord.z,true) << od_tab;
	strm << mConvertVal(coord.z-kbdepth,true) << od_tab;
	strm << mConvertVal(track_.dah(idx),true) << od_newline;
    }
}


// ==================================================================


static const int cMDCol = 0;
static const int cTVDCol = 1;

#define mD2TModel (cksh_ ? wd_.checkShotModel() : wd_.d2TModel())


uiD2TModelDlg::uiD2TModelDlg( uiParent* p, Well::Data& wd, bool cksh )
	: uiDialog(p,mGetDlgSetup(wd,mTDName(cksh),mD2TModelDlgHelpID))
	, wd_(wd)
	, cksh_(cksh)
	, orgd2t_(mD2TModel ? new Well::D2TModel(*mD2TModel) : 0)
	, origreplvel_(wd.info().replvel)
	, replvelfld_(0)
{
    tbl_ = new uiTable( this, uiTable::Setup()
				.rowdesc(cksh_ ? "Measure point" : "Control Pt")
				.rowgrow(true)
				.insertrowallowed(true)
				.removerowallowed(true)
				.defrowlbl(true)
				.selmode(uiTable::Multi)
				.removeselallowed(true),
				(mTDName(cksh)).getFullString().buf() );

    timefld_ = new uiCheckBox( this, tr(" Time is TWT") );
    timefld_->setChecked( true );
    timefld_->activated.notify( mCB(this,uiD2TModelDlg,fillTable) );

    zinftfld_ = new uiCheckBox( this, tr(" Z in feet") );
    zinftfld_->setChecked( SI().depthsInFeet() );
    zinftfld_->activated.notify( mCB(this,uiD2TModelDlg,fillTable) );

    BufferStringSet header;
    getColLabels( header );
    tbl_->setColumnLabels( header );
    tbl_->setNrRows( nremptyrows );
    tbl_->valueChanged.notify( mCB(this,uiD2TModelDlg,dtpointChangedCB) );
    tbl_->rowDeleted.notify( mCB(this,uiD2TModelDlg,dtpointRemovedCB) );
    uiString kbstr = toUiString(Well::Info::sKBElev());
    tbl_->setColumnToolTip( cMDCol,
	   tr("Measured depth along the borehole, origin at %1").arg(kbstr));
    tbl_->setColumnToolTip( cTVDCol,
	    tr("True Vertical Depth, origin at %1").arg(kbstr) );
    tbl_->setColumnToolTip( getTVDSSCol(),
	    tr("True Vertical Depth Sub-Sea, positive downwards") );
    tbl_->setColumnToolTip( getTimeCol(),
	 tr("Two-way travel times with same origin as seismic survey: SRD=0") );
    tbl_->setColumnToolTip( getVintCol(),
	    tr("Interval velocity above this control point (read-only)") );
    tbl_->setPrefWidth( 700 );
    tbl_->setTableReadOnly( !writable_ );
    tbl_->setSelectionBehavior( uiTable::SelectRows );

    timefld_->attach( rightAlignedBelow, tbl_ );
    zinftfld_->attach( leftOf, timefld_ );

    if ( !cksh_ )
    {
	replvelfld_ = new uiGenInput(this,Well::Info::sReplVel(),
				      FloatInpSpec(mUdf(float)) );
	if ( !writable_ )
	    replvelfld_-> setReadOnly( true );
	else
	{
	    uiButton* updbut = new uiPushButton( this, tr("Update display"),
				       mCB(this,uiD2TModelDlg,updNow), true );
	    updbut->attach( ensureBelow, tbl_ );
	    replvelfld_->updateRequested.notify(
					mCB(this,uiD2TModelDlg,updReplVelNow) );
	    uiPushButton* setbut = new uiPushButton( this, tr("Set"),
			      mCB(this,uiD2TModelDlg,updReplVelNow), true );
	    setbut->attach( rightOf, replvelfld_ );
	}
	replvelfld_->attach( ensureBelow, zinftfld_ );
	zinftfld_->activated.notify( mCB(this,uiD2TModelDlg,fillReplVel) );
    }

    uiGroup* iobutgrp = new uiButtonGroup( this, "Input/output buttons",
					   OD::Horizontal );
    if ( writable_ )
	new uiPushButton( iobutgrp, uiStrings::sImport(),
	    mCB(this,uiD2TModelDlg,readNew), false );
    new uiPushButton( iobutgrp, uiStrings::sExport(),
	mCB(this,uiD2TModelDlg,expData), false );
    if ( replvelfld_ )
	iobutgrp->attach( ensureBelow, replvelfld_ );
    else
	iobutgrp->attach( ensureBelow, zinftfld_ );

    correctD2TModelIfInvalid();

    fillTable(0);
    if ( !cksh_ )
	fillReplVel(0);
}


void uiD2TModelDlg::getColLabels( BufferStringSet& lbls ) const
{
    const bool zinfeet = zinftfld_ ? zinftfld_->isChecked() : false;
    const bool timeisoneway = timefld_ ? !timefld_->isChecked() : false;
    const BufferString depthunit = getDistUnitString( zinfeet, true );

    BufferString curlbl( sKeyMD() );
    curlbl.addSpace().add( depthunit );
    lbls.add( curlbl );

    curlbl.set( sKeyTVD() );
    curlbl.addSpace().add( depthunit );
    lbls.add( curlbl );

    if ( !mIsUdf(getTVDGLCol()) )
    {
	curlbl.set( sKeyTVDGL() );
	curlbl.addSpace().add( depthunit );
	lbls.add( curlbl );
    }

    curlbl.set( sKeyTVDSS() );
    curlbl.addSpace().add( depthunit );
    lbls.add( curlbl );

    if ( !mIsUdf(getTVDSDCol()) )
    {
	curlbl.set( sKeyTVDSD() );
	curlbl.addSpace().add( depthunit );
	lbls.add( curlbl );
    }

    curlbl.set( timeisoneway ? sKeyOWT() : sKeyTWT() );
    curlbl.addSpace().add(
	    UnitOfMeasure::surveyDefTimeUnitAnnot(true,true).getFullString() );
    lbls.add( curlbl );

    curlbl.set( sKeyVint() );
    curlbl.addSpace().add( getVelUnitString(zinfeet,true) );
    lbls.add( curlbl );
}


int uiD2TModelDlg::getTVDGLCol() const
{
    return mIsUdf( wd_.info().groundelev ) ? mUdf( int ) : cTVDCol + 1;
}


int uiD2TModelDlg::getTVDSSCol() const
{
    return mIsUdf( getTVDGLCol() ) ? cTVDCol + 1 : getTVDGLCol() + 1;
}


int uiD2TModelDlg::getTVDSDCol() const
{
    return mIsZero(SI().seismicReferenceDatum(),1e-3) ? mUdf( int )
						      : getTVDSSCol() + 1;
}


int uiD2TModelDlg::getTimeCol() const
{
    return mIsUdf( getTVDSDCol() ) ? getTVDSSCol() + 1
				   : getTVDSDCol() + 1;
}


int uiD2TModelDlg::getVintCol() const
{
    return getTimeCol() + 1;
}


void uiD2TModelDlg::setDepthValue( int irow, int icol, float val )
{
    if ( icol == getTimeCol() ) return;
    tbl_->setValue( RowCol(irow,icol), mConvertVal(val,true), 2 );
}


float uiD2TModelDlg::getDepthValue( int irow, int icol ) const
{
    if ( icol == getTimeCol() ) return mUdf(float);
    return mConvertVal( tbl_->getfValue( RowCol(irow,icol) ), false );
}


#define mConvertTimeVal(val,toscreen) ( \
	toscreen ? \
	mDataUom(false)->userValue(val) / (1.f + (int)!timefld_->isChecked()): \
	mDataUom(false)->internalValue( val*(1.f+(int)!timefld_->isChecked())) )

void uiD2TModelDlg::setTimeValue( int irow, float val )
{
    tbl_->setValue( RowCol(irow,getTimeCol()), mConvertTimeVal(val,true), 2 );
}


float uiD2TModelDlg::getTimeValue( int irow ) const
{
    return mConvertTimeVal(tbl_->getfValue(RowCol(irow,getTimeCol())),false);
}

#define mGetVel(dah,d2t) \
{ \
    Interval<float> replvellayer( wd_.track().getKbElev(), srd ); \
    replvellayer.widen( 1e-2f, true ); \
    vint = replvellayer.includes( -1.f * wd_.track().getPos(dah).z, true ) && \
	   !mIsUdf(wd_.info().replvel) \
	 ? wd_.info().replvel \
	 : mCast(float,d2t->getVelocityForDah( dah, wd_.track() )); \
}


void uiD2TModelDlg::fillTable( CallBacker* )
{
    NotifyStopper ns( tbl_->valueChanged );
    tbl_->setColumnReadOnly( getVintCol(), false );
    const Well::D2TModel* d2t = mD2TModel;
    const Well::Track& track = wd_.track();
    const int tracksz = wd_.track().size();
    if ( !d2t || d2t->size()<2 )
	return;

    if ( tracksz<2 )
    {
	uiMSG().error( tr("Invalid track") );
	return;
    }

    const int dtsz = d2t->size();
    tbl_->setNrRows( dtsz + nremptyrows );
    BufferStringSet header;
    getColLabels( header );
    tbl_->setColumnLabels( header );

    const float kbelev = wd_.track().getKbElev();
    const float groundevel = wd_.info().groundelev;
    const float srd = mCast(float,SI().seismicReferenceDatum());
    const bool hastvdgl = !mIsUdf( groundevel );
    const bool hastvdsd = !mIsZero( srd, 1e-3f );
    float vint;
    for ( int idx=0; idx<dtsz; idx++ )
    {
	const float dah = d2t->dah(idx);
	const float tvdss = mCast(float,track.getPos(dah).z);
	const float tvd = tvdss + kbelev;
	mGetVel(dah,d2t)
	setDepthValue( idx, cMDCol, dah );
	setDepthValue( idx, cTVDCol, tvd );
	if ( hastvdgl )
	    setDepthValue( idx, getTVDGLCol(),	tvdss + groundevel );

	if ( hastvdsd )
	    setDepthValue( idx, getTVDSDCol(), tvdss + srd );

	setDepthValue( idx, getTVDSSCol(), tvdss );
	setTimeValue( idx, d2t->t(idx) );
	setDepthValue( idx, getVintCol(), vint );
    }
    tbl_->setColumnReadOnly( getVintCol(), true );
}


void uiD2TModelDlg::fillReplVel( CallBacker* )
{
    NotifyStopper ns( replvelfld_->updateRequested );
    uiString lbl = toUiString("%1 %2").arg(Well::Info::sKeyReplVel()).arg(
				VelocityDesc::getVelUnit(true));
    if ( zinftfld_->isChecked() ) lbl = toUiString("%1 ").arg(lbl);
    replvelfld_->setTitleText( lbl );
    replvelfld_->setValue( mConvertVal(wd_.info().replvel,true) );
}


uiD2TModelDlg::~uiD2TModelDlg()
{
    delete orgd2t_;
}


void uiD2TModelDlg::dtpointChangedCB( CallBacker* )
{
    const int row = tbl_->currentRow();
    const int col = tbl_->currentCol();

    if ( col < getTimeCol() )
	updateDtpointDepth( row );
    else if ( col == getTimeCol() )
	updateDtpointTime( row );
}


void uiD2TModelDlg::dtpointRemovedCB( CallBacker* )
{
    Well::D2TModel* d2t = mD2TModel;
    if ( !d2t || d2t->size()<3 )
    {
	uiMSG().error(tr("Invalid time-depth model"));
	return;
    }

    const int row = tbl_->currentRow();
    int idah = d2t->indexOf( getDepthValue(row,cMDCol) );
    if ( ( rowIsIncomplete(row) && !mIsUdf(getNextCompleteRowIdx(row)) ) ||
	 mIsUdf(idah) )
	return;

    d2t->remove( idah-1 );
    const int nextrow = getNextCompleteRowIdx( row-1 );
    if ( mIsUdf(nextrow) )
	return;

    idah = d2t->indexOf( getDepthValue(nextrow,cMDCol) );
    const float olddah = d2t->dah( idah );
    updateDtpoint( nextrow, olddah );
}


bool uiD2TModelDlg::updateDtpointDepth( int row )
{
    NotifyStopper ns( tbl_->valueChanged );
    const Well::D2TModel* d2t = mD2TModel;
    const Well::Track& track = wd_.track();
    const int tracksz = wd_.track().size();
    if ( !d2t || d2t->size()<2 || tracksz<2 )
    {
	uiString errmsg = tracksz<2 ? tr("Invalid track")
				    : tr("Invalid time-depth model");
	mErrRet(errmsg)
    }

    const bool newrow = rowIsIncomplete( row );

    int incol = tbl_->currentCol();
    const bool md2tvd = incol == cMDCol;
    const bool inistvd = incol == cTVDCol;
    bool inistvdss = incol == getTVDSSCol();
    if ( incol == getTimeCol() )
    {
	inistvdss = true; // special case for replacement velocity zone update
	incol = getTVDSSCol();
    }

    const float kbelev = wd_.track().getKbElev();
    const float groundevel = wd_.info().groundelev;
    const bool hastvdgl = !mIsUdf( groundevel );
    const bool inistvdgl = hastvdgl && incol == getTVDGLCol();

    const float srd = mCast(float,SI().seismicReferenceDatum());
    const bool hastvdsd = !mIsZero( srd, 1e-3f );
    const bool inistvdsd = hastvdsd && incol == getTVDSDCol();

    const float olddah = newrow ? mUdf(float) : d2t->dah( row );
    const float oldtvdss = newrow ? mUdf(float) : (float)track.getPos(olddah).z;
    float oldval = mUdf( float );
    if ( inistvd && !mIsUdf(oldtvdss) )
	oldval = oldtvdss + kbelev;
    else if ( inistvdgl && !mIsUdf(oldtvdss) )
	oldval = oldtvdss + groundevel;
    else if ( inistvdsd && !mIsUdf(oldtvdss) )
	oldval = oldtvdss + srd;
    else if ( inistvdss && !mIsUdf(oldtvdss) )
	oldval = oldtvdss;

    const RowCol rcin(row,incol);
    if ( mIsUdf(getDepthValue(row,incol)) )
    {
	uiMSG().error( tr("Please enter a valid number") );
	if ( !newrow )
	    setDepthValue( row, incol, oldval );

	return false;
    }

    float inval = getDepthValue( row, incol );
    Interval<float> dahrg = track.dahRange();
    Interval<float> zrg( track.value( 0 ), track.value( tracksz - 1 ) );
    if ( inistvd )
	zrg.shift( kbelev );
    else if ( inistvdgl )
	zrg.shift( groundevel );
    else if ( inistvdsd )
	zrg.shift( srd );

    const Interval<float>& zrange = md2tvd ? dahrg : zrg;
    Interval<float> tblrg;
    const int prevrow = getPreviousCompleteRowIdx( row );
    const int nextrow = getNextCompleteRowIdx( row );
    tblrg.start = row == 0 || mIsUdf(prevrow) ? zrange.start
	       : getDepthValue( prevrow, incol );
    tblrg.stop = d2t->size() < row || mIsUdf(nextrow) ? zrange.stop
	       : getDepthValue( nextrow, incol );

    BufferString lbl;
    if ( md2tvd ) lbl = sKeyMD();
    else if ( inistvd ) lbl =  sKeyTVD();
    else if ( inistvdgl ) lbl = sKeyTVDGL();
    else if ( inistvdsd ) lbl = sKeyTVDSD();
    else if ( inistvdss ) lbl = sKeyTVDSS();

    uiString errmsg = tr("The entered %1");
    if ( !zrange.includes(inval,true) )
    {
	errmsg.arg(tr("%1 value %2 is outside of track range\n[%3, %4]%5 %6")
	      .arg( lbl )
	      .arg( mConvertVal(inval,true) )
	      .arg( mConvertVal(zrange.start,true) )
	      .arg( mConvertVal(zrange.stop,true) )
	      .arg(getDistUnitString(zinftfld_->isChecked(),true)).arg(lbl));
	setDepthValue( row, incol, !newrow ? oldval : mUdf(float) );
	mErrRet(errmsg)
    }

    if ( !tblrg.includes(inval,true) )
    {
	errmsg.arg(tr("%1 is not between the depths of the previous and "
		      "next control points").arg(lbl));
	setDepthValue( row, incol, !newrow ? oldval : mUdf(float) );
	mErrRet(errmsg)
    }

    if ( !md2tvd )
    {
	if ( inistvd ) inval -= kbelev;
	else if ( inistvdgl ) inval -= groundevel;
	else if ( inistvdsd ) inval -= srd;
    }

    const float dah = md2tvd ? inval : track.getDahForTVD( inval );
    const float tvdss = !md2tvd ? inval : mCast(float,track.getPos(inval).z);
    if ( !md2tvd )
	setDepthValue( row, cMDCol, dah );

    if ( !inistvdss )
	setDepthValue( row, getTVDSSCol(), tvdss );

    if ( !inistvd )
	setDepthValue( row, cTVDCol, tvdss + kbelev );

    if ( hastvdgl && !inistvdgl )
	setDepthValue( row, getTVDGLCol(), tvdss + groundevel );

    if ( hastvdsd && !inistvdsd )
	setDepthValue( row, getTVDSDCol(), tvdss + srd );

    Interval<float> replvelint( track.getKbElev(), srd );
    if ( replvelint.includes(-1.f*tvdss,true) && !mIsUdf(wd_.info().replvel) )
    {
	const float twt = (tvdss + srd ) / wd_.info().replvel;
	setTimeValue( row, twt );
    }

    return updateDtpoint( row, olddah );
}


bool uiD2TModelDlg::updateDtpointTime( int row )
{
    NotifyStopper ns( tbl_->valueChanged );
    const Well::D2TModel* d2t = mD2TModel;
    const Well::Track& track = wd_.track();
    const int tracksz = wd_.track().size();
    if ( !d2t || d2t->size()<2 || tracksz<2 )
    {
	uiString errmsg = tracksz<2 ? tr("Invalid track")
				    : tr("Invalid time-depth model");
	mErrRet(errmsg)
    }

    const bool newrow = rowIsIncomplete( row );
    const float oldval = newrow ? mUdf(float) : d2t->value( row );
    if ( mIsUdf(getTimeValue(row)) )
    {
	uiMSG().error( tr("Please enter a valid number") );
	if ( !newrow )
	    setTimeValue( row, oldval );

	return false;
    }

    const float inval = getTimeValue( row );
    if ( inval < 0.f )
    {
	uiString errmsg = tr("Negative travel-times are not allowed"
			     "The minimim allowed travel-time is zero" );
	setTimeValue( row, !newrow ? oldval : mUdf(float) );
	mErrRet(errmsg)
    }

    Interval<float> dahrg = track.dahRange();
    Interval<float> timerg;
    timerg.start = d2t->getTime( dahrg.start, track );
    timerg.stop = d2t->getTime( dahrg.stop, track );
    Interval<float> tblrg;
    const int prevrow = getPreviousCompleteRowIdx( row );
    const int nextrow = getNextCompleteRowIdx( row );
    tblrg.start = row == 0 || mIsUdf(prevrow) ? timerg.start
	       : getTimeValue( prevrow );
    tblrg.stop = d2t->size() < row || mIsUdf(nextrow) ? timerg.stop
	       : getTimeValue( nextrow );

    if ( !tblrg.includes(inval,true) &&
	 !mIsUdf(getPreviousCompleteRowIdx(row)) &&
	 !mIsUdf(getNextCompleteRowIdx(row)) )
    {
	uiString errmsg = tr("The entered time is not between the times "
			     "of the previous and next control points" );
	setTimeValue( row , !newrow ? oldval : mUdf(float) );
	mErrRet(errmsg)
    }

    return updateDtpoint( row, oldval );
}


bool uiD2TModelDlg::updateDtpoint( int row, float oldval )
{
    NotifyStopper ns( tbl_->valueChanged );
    tbl_->setColumnReadOnly( getVintCol(), false );
    Well::D2TModel* d2t = mD2TModel;
    const Well::Track& track = wd_.track();
    const int tracksz = wd_.track().size();
    if ( !d2t || d2t->size()<2 || tracksz<2 )
    {
	uiString errmsg = tracksz<2 ? tr("Invalid track")
				    : tr("Invalid time-depth model");
	mErrRet(errmsg)
    }

    if ( mIsUdf(getDepthValue(row,cMDCol)) || mIsUdf(getTimeValue(row)) )
	return true; // not enough data yet to proceed

    if ( !rowIsIncomplete(row) )
    {
	const bool oldvalisdah = tbl_->currentCol() < getTimeCol();
	const float olddah = oldvalisdah ? oldval : d2t->getDah( oldval, track);
	const int dahidx = d2t->indexOf( olddah );
	d2t->remove( dahidx );
    }

    const float dah = getDepthValue( row, cMDCol );
    const float twt = getTimeValue( row );
    d2t->insertAtDah( dah, twt );
    wd_.d2tchanged.trigger();

    const float srd = mCast(float,SI().seismicReferenceDatum());
    float vint;
    mGetVel(dah,d2t);
    setDepthValue( row, getVintCol(), vint );

    for ( int irow=row+1; irow<tbl_->nrRows(); irow++ )
    {
	if ( rowIsIncomplete(irow) )
	    continue;

	const float nextdah = getDepthValue( irow, cMDCol );
	mGetVel(nextdah,d2t);
	setDepthValue( irow, getVintCol(), vint );
	break;
    }

    tbl_->setColumnReadOnly( getVintCol(), true );
    return true;
}


bool uiD2TModelDlg::rowIsIncomplete( int row ) const
{
    Well::D2TModel* d2t = mD2TModel;
    if ( !d2t || d2t->size()<2 )
	mErrRet( tr("Invalid time-depth model") )

    if ( row >= d2t->size() )
	return true;

    return mIsUdf( getDepthValue( row, getVintCol() ) );
}


int uiD2TModelDlg::getPreviousCompleteRowIdx( int row ) const
{
    for ( int irow=row-1; irow>=0; irow-- )
    {
	if ( rowIsIncomplete(irow) )
	    continue;

	return irow;
    }

    return mUdf(int);
}


int uiD2TModelDlg::getNextCompleteRowIdx( int row ) const
{
    for ( int irow=row+1; irow<tbl_->nrRows(); irow++ )
    {
	if ( rowIsIncomplete(irow) )
	    continue;

	return irow;
    }

    return mUdf(int);
}



class uiD2TModelReadDlg : public uiDialog
{ mODTextTranslationClass(uiD2TModelReadDlg);
public:

uiD2TModelReadDlg( uiParent* p, Well::Data& wd, bool cksh )
	: uiDialog(p,uiDialog::Setup( mTDOpName(uiStrings::sImport(),cksh),
		     mNoDlgTitle, mODHelpKey(mD2TModelReadDlgHelpID) ))
	, cksh_(cksh)
	, wd_(wd)
{
    setOkText( uiStrings::sImport() );
    uiD2TModelGroup::Setup su( false );
    su.filefldlbl( "File name" );
    d2tgrp = new uiD2TModelGroup( this, su );
}


bool acceptOK( CallBacker* )
{
    if ( !d2tgrp->getD2T(wd_,cksh_) )
    {
	if ( !d2tgrp->errMsg().isEmpty() )
	    uiMSG().error( d2tgrp->errMsg() );

	return false;
    }
    if ( !d2tgrp->warnMsg().isEmpty() )
	uiMSG().warning( d2tgrp->warnMsg() );

    Well::D2TModel* d2t = mD2TModel;
    if ( d2t && d2t->size()>1 )
	d2t->deInterpolate();

    return true;
}

    uiD2TModelGroup*	d2tgrp;
    Well::Data&		wd_;
    const bool		cksh_;

};


void uiD2TModelDlg::readNew( CallBacker* )
{
    uiD2TModelReadDlg dlg( this, wd_, cksh_ );
    if ( !dlg.go() ) return;

    if ( !dlg.d2tgrp->getD2T(wd_,cksh_) )
	return;
    else
    {
	tbl_->clearTable();
	fillTable(0);
	wd_.d2tchanged.trigger();
    }
}


void uiD2TModelDlg::expData( CallBacker* )
{
    Well::D2TModel* d2t = mD2TModel;
    getModel( *d2t );
    if ( !d2t || d2t->size() < 2 )
	{ uiMSG().error( tr("No valid data entered") ); return; }

    uiFileDialog dlg( this, false, 0, 0, tr("Filename for export") );
    dlg.setDirectory( GetDataDir() );
    if ( !dlg.go() )
	return;

    const BufferString fnm( dlg.fileName() );
    od_ostream strm( fnm );
    if ( !strm.isOK() )
    {
	uiMSG().error( tr("Cannot open '%1' for write").arg(fnm) );
	return;
    }

    const float kbelev = mConvertVal( wd_.track().getKbElev(), true );
    const float groundevel = mConvertVal( wd_.info().groundelev, true );
    const float srd = mConvertVal( mCast(float,SI().seismicReferenceDatum()),
				   true );
    const bool hastvdgl = !mIsUdf( groundevel );
    const bool hastvdsd = !mIsZero( srd, 1e-3f );
    BufferStringSet header;
    getColLabels( header );

    strm << header.get( cMDCol ) << od_tab <<  header.get( cTVDCol ) << od_tab;
    if ( hastvdgl )
	strm << header.get( getTVDGLCol() ) << od_tab;

    strm << header.get( getTVDSSCol() ) << od_tab;
    if ( hastvdsd )
	strm << header.get( getTVDSDCol() ) << od_tab;

    strm << header.get( getTimeCol() ) << od_tab;
    strm << header.get( getVintCol() ) << od_newline;
    float vint;
    for ( int idx=0; idx<d2t->size(); idx++ )
    {
	const float dah = d2t->dah(idx);
	const float tvdss = mConvertVal(
				mCast(float,wd_.track().getPos(dah).z), true );
	const float tvd = tvdss + kbelev;
	mGetVel(dah,d2t);
	strm << mConvertVal(dah,true) << od_tab << tvd << od_tab;
	if ( hastvdgl )
	{
	    const float tvdgl = tvdss + groundevel;
	    strm << tvdgl << od_tab;
	}
	strm << tvdss << od_tab;
	if ( hastvdsd )
	{
	    const float tvdsd = tvdss + srd;
	    strm << tvdsd << od_tab;
	}
	strm << mConvertTimeVal( d2t->t(idx), true ) << od_tab;
	strm << mConvertVal( vint, true ) << od_newline;
    }
}


bool uiD2TModelDlg::getFromScreen()
{
    Well::D2TModel* d2t = mD2TModel;
    getModel( *d2t );

    if ( wd_.track().zRange().stop < SI().seismicReferenceDatum() && !d2t )
	return true;

    if ( !d2t || d2t->size() < 2 )
	mErrRet( tr("Please define at least two control points.") )

    return true;
}


void uiD2TModelDlg::updNow( CallBacker* )
{
    if ( !getFromScreen() )
	return;

    Well::D2TModel* d2t = mD2TModel;
    if ( d2t && d2t->size() > 1 )
	wd_.d2tchanged.trigger();

    wd_.trackchanged.trigger();
}


void uiD2TModelDlg::updReplVelNow( CallBacker* )
{
    const float replvel = mConvertVal(replvelfld_->getfValue(),false);
    if ( mIsUdf(replvel) || replvel < 0.001f )
    {
	uiMSG().error( tr("Please enter a valid %1")
		       .arg(Well::Info::sReplVel()) );
	replvelfld_->setValue( mConvertVal(wd_.info().replvel,true) );
	return;
    }

    Well::D2TModel* d2t = mD2TModel;
    const Well::Track& track = wd_.track();
    if ( track.zRange().stop < SI().seismicReferenceDatum() &&
	 ( !d2t || d2t->size() < 2 ) )
    { //Whole well track is above SRD: The entire model is controlled by replvel
	wd_.info().replvel = replvel;
	updNow(0);
	return;
    }

    const int tracksz = wd_.track().size();
    if ( !d2t || d2t->size()<2 || tracksz<2 )
    {
	uiString errmsg = tracksz<2 ? tr("Invalid track")
				    : tr("Invalid time-depth model");
	uiMSG().error( errmsg );
	return;
    }

    const float kbelev = track.getKbElev();
    const float srdelev = mCast(float,SI().seismicReferenceDatum());
    const bool kbabovesrd = srdelev < kbelev;
    const float firstdah = kbabovesrd ? track.getDahForTVD(-1.f*srdelev) : 0.f;
    const float firsttwt = d2t->getTime( firstdah, track );
    if ( mIsUdf(firstdah) || mIsUdf(firsttwt) )
	return;

    const float zwllhead = track.value(0);
    const float replveldz = zwllhead + srdelev;
    const float timeshift = kbabovesrd ? firsttwt
				       : 2.f * replveldz / replvel - firsttwt;
    wd_.info().replvel = replvel;
    if ( mIsZero(timeshift,1e-2f) )
    {
	const float dah = getDepthValue( 0, cMDCol );
	Interval<float> replvellayer( kbelev, srdelev );
	replvellayer.widen( 1e-2f, true );
	if ( replvellayer.includes( -1.f * wd_.track().getPos(dah).z, true ) )
	    setDepthValue( 0, getVintCol(), replvel );

	return;
    }

    for( int irow=tbl_->nrRows()-1; irow>=0; irow-- )
    {
	const float dah = getDepthValue( irow, cMDCol );
	const float twt = getTimeValue( irow );
	if ( mIsUdf(dah) || mIsUdf(twt) )
	    continue;

	if ( dah < firstdah || twt + timeshift < 0.f)
	    tbl_->removeRow( irow );
    }

    NotifyStopper ns( tbl_->valueChanged );
    for( int irow=0; irow<tbl_->nrRows(); irow++ )
    {
	const float twt = getTimeValue( irow );
	if ( mIsUdf(twt) )
	    continue;

	setTimeValue( irow, twt + timeshift );
    }

    updNow(0);
}


void uiD2TModelDlg::getModel( Well::D2TModel& d2t )
{
    d2t.setEmpty();
    const int nrrows = tbl_->nrRows();
    for ( int irow=0; irow<nrrows; irow++ )
    {
	if ( mIsUdf(getDepthValue(irow,getVintCol())) )
	    continue;

	const float dah = getDepthValue( irow, cMDCol );
	const float twt = getTimeValue( irow );
	d2t.add( dah, twt );
    }
}


bool uiD2TModelDlg::rejectOK( CallBacker* )
{
    Well::D2TModel* d2t = mD2TModel;
    if ( d2t )
	*d2t = *orgd2t_;

    wd_.d2tchanged.trigger();
    wd_.trackchanged.trigger();
    wd_.info().replvel = origreplvel_;

    return true;
}


void uiD2TModelDlg::correctD2TModelIfInvalid()
{
    Well::D2TModel* d2t = mD2TModel;
    if ( !d2t || d2t->size() < 2 )
	return;

    bool needrestore = false;
    uiString errmsg = tr("Invalid model detected.");
    if ( !d2t->ensureValid(wd_,errmsg) || d2t->size() < 2 )
    {
	uiString msg = tr("%1\n"
			  "Could not autocorrect the current model.")
			  .arg(errmsg);
	uiMSG().warning( msg );
	if ( *d2t != *orgd2t_ )
	    needrestore = true;
    }
    else
    {
	if ( *d2t != *orgd2t_ )
	{
	    uiString msg = tr("%1\nAuto-correct?\n"
		"(New model will only be saved on disk on successful exit of"
		" the editor)")
		.arg(errmsg);
	    needrestore = !uiMSG().askGoOn( msg );
	    if ( !needrestore )
		uiMSG().message( tr("Time-depth model succesfully corrected") );
	}
    }

    if ( needrestore )
	*d2t = *orgd2t_;
}


bool uiD2TModelDlg::acceptOK( CallBacker* )
{
    if ( !writable_ )
	return true;

    if ( !getFromScreen() )
	return false;

    wd_.d2tchanged.trigger();
    wd_.trackchanged.trigger();
    return true;
}


//============================================================================

uiNewWellDlg::uiNewWellDlg( uiParent* p )
    : uiGetObjectName(p,uiGetObjectName::Setup(tr("New Well"),mkWellNms())
				.inptxt(tr("New well name")) )
{
    setHelpKey( mODHelpKey(mNewWellTrackDlgHelpID) );

    if ( listfld_ )
    {
	uiLabel* lbl = new uiLabel( this, tr("Existing wells") );
	lbl->attach( leftOf, listfld_ );
    }

    colsel_ = new uiColorInput( this, uiColorInput::Setup(getRandStdDrawColor())
				      .lbltxt(uiStrings::sColor()) );
    colsel_->attach( alignedBelow, inpFld() );
}


uiNewWellDlg::~uiNewWellDlg()
{
    delete nms_;
}


const BufferStringSet& uiNewWellDlg::mkWellNms()
{
    nms_ = new BufferStringSet;
    IOObjContext ctxt( WellTranslatorGroup::ioContext() );
    const IODir iodir( ctxt.getSelKey() );
    const IODirEntryList del( iodir, ctxt );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IOObj* ioobj = del[idx]->ioobj_;
	if ( ioobj )
	    nms_->add( ioobj->name() );
    }
    return *nms_;
}


bool uiNewWellDlg::acceptOK( CallBacker* )
{
    BufferString newnm( text() );
    if ( newnm.trimBlanks().isEmpty() )
	mErrRet( tr("Please enter a name") )

    bool res = true;
    if ( nms_->isPresent(newnm) )
    {
	res = uiMSG().askOverwrite( tr("Well name already exists. "
		"Do you want to overwrite?") );
    }

    if ( !res )
	return false;

    name_ = newnm;
    return true;
}


const Color& uiNewWellDlg::getWellColor()
{
    return colsel_->color();
}


//============================================================================

static const char* collbls[] = { "Well name","Log name",
				 "Unit of measure", 0 };
uiWellLogUOMDlg::uiWellLogUOMDlg( uiParent* p, ObjectSet<Well::LogSet> wls,
				  const BufferStringSet wellnms,
				  const BufferStringSet lognms )
    : uiDialog(p,uiDialog::Setup(tr("Set units of measure for logs"),
				 mNoDlgTitle,mNoHelpKey))
{
    fillTable( wls, wellnms, lognms );
}


void uiWellLogUOMDlg::fillTable( ObjectSet<Well::LogSet> wls,
			    const BufferStringSet& wellnms,
			    const BufferStringSet& lognms )
{
    uominfotbl_ = new uiTable( this, uiTable::Setup()
				    .manualresize(true)
				    .fillrow(true)
				    .removeselallowed(false),
				"Units info" );
    uominfotbl_->setPrefWidth( 520 );
    uominfotbl_->setPrefHeight( 400 );
    uominfotbl_->setTableReadOnly( true );
    uominfotbl_->setColumnLabels( collbls );
    uominfotbl_->setColumnResizeMode( uiTable::ResizeToContents );
    const int nrwls = wls.size();
    const int nrlogs = lognms.size();
    const int nrrows = nrwls*nrlogs;
    uominfotbl_->setNrRows( nrrows );
    int rowidx = -1;
    for ( int wlsidx=0; wlsidx<nrwls; wlsidx++ )
    {
	for ( int lidx=0; lidx<nrlogs; lidx++ )
	{
	    rowidx++;
	    Well::Log* log = wls[wlsidx]->getLog( lognms.get(lidx) );
	    if ( !log )
	    {
		uominfotbl_->removeRow( rowidx );
		rowidx--;
		continue;
	    }

	    logs_ += log;
	    const char* curruom = log->unitMeasLabel();
	    const UnitOfMeasure* uom = UnitOfMeasure::getGuessed( curruom );

	    PropertyRef::StdType ptyp = PropertyRef::Other;
	    if ( uom ) ptyp = uom->propType();
	    uiUnitSel::Setup ussu( ptyp, uiStrings::sEmptyString() );
	    ussu.selproptype( true );
	    ussu.withnone_ = true;
	    uiUnitSel* unfld = new uiUnitSel( 0, ussu );
	    unfld->setUnit( uom );
	    unflds_ += unfld;
	    uominfotbl_->setText( RowCol(rowidx,0), wellnms.get(wlsidx) );
	    uominfotbl_->setText( RowCol(rowidx,1), lognms.get(lidx ) );
	    uominfotbl_->setCellGroup( RowCol(rowidx,2), unfld );
	}
    }
}


bool uiWellLogUOMDlg::setUoMValues()
{
    const int logssz = logs_.size();
    if ( !logssz || logssz!=uominfotbl_->nrRows() )
    {
	uiMSG().message( tr("No logs found.") );
	return false;
    }

    const int nrrows = uominfotbl_->nrRows();
    for ( int lidx=0; lidx<nrrows; lidx++ )
    {
	Well::Log* log = logs_[lidx];
	if ( !log )
	    continue;

	const UnitOfMeasure* newuom = unflds_[lidx]->getUnit();
	log->setUnitMeasLabel( newuom ? newuom->name().buf() : 0 );
    }

    return true;
}


bool uiWellLogUOMDlg::acceptOK( CallBacker* )
{
    return setUoMValues();
}



// uiSetD2TFromOtherWell
uiSetD2TFromOtherWell::uiSetD2TFromOtherWell( uiParent* p )
    : uiDialog(p,Setup(tr("Set Depth-Time Model"),mNoDlgTitle,mTODOHelpKey))
{
    inpwellfld_ = new uiWellSel( this, true, tr("Use D2T model from"), false );

    uiIOObjSelGrp::Setup su( OD::ChooseAtLeastOne );
    su.withinserters(false).withwriteopts(false);
    wellfld_ = new uiMultiWellSel( this, false, &su );
    wellfld_->allowIOObjManip( false );
    wellfld_->attach( alignedBelow, inpwellfld_ );
    uiLabel* lbl = new uiLabel( this, tr("Apply to") );
    lbl->attach( centeredLeftOf, wellfld_ );
}


uiSetD2TFromOtherWell::~uiSetD2TFromOtherWell()
{
}


void uiSetD2TFromOtherWell::setSelected( const TypeSet<MultiID>& keys )
{
    wellfld_->setSelected( keys );
}


bool uiSetD2TFromOtherWell::acceptOK( CallBacker* )
{
    const IOObj* inpioobj = inpwellfld_->ioobj();
    if ( !inpioobj )
	return false;

    RefMan<Well::Data> inpwd = new Well::Data;
    PtrMan<Well::Reader> inprdr = new Well::Reader( *inpioobj, *inpwd );
    if ( !inprdr->getD2T() )
    {
	uiMSG().message( tr("Can not read input model.") );
	return false;
    }

    const Well::D2TModel* d2t = inpwd->d2TModel();
    TimeDepthModel dtmodel;
    if ( !d2t || !d2t->getTimeDepthModel(*inpwd,dtmodel) )
    {
	uiMSG().error( tr("Input Depth-Time Model is invalid") );
	return false;
    }

    TypeSet<MultiID> selwells;
    wellfld_->getSelected( selwells );
    if ( selwells.isEmpty() )
    {
	uiMSG().error( tr("Please select at least one target well") );
	return false;
    }

    const int mdlsz = dtmodel.size();
    TypeSet<double> inputdepths( mdlsz, 0. );
    TypeSet<double> inputtimes( mdlsz, 0. );
    for ( int idx=0; idx<mdlsz; idx++ )
    {
	inputdepths[idx] = dtmodel.getDepth( idx );
	inputtimes[idx] = dtmodel.getTime( idx );
    }

    uiStringSet errmsgs;
    for ( int idx=0; idx<selwells.size(); idx++ )
    {
	RefMan<Well::Data> wd = new Well::Data;
	PtrMan<Well::Reader> rdr = new Well::Reader( selwells[idx], *wd );
	if ( !rdr->getD2T() )
	    continue;

	TypeSet<double> depths( inputdepths );
	TypeSet<double> times( inputtimes );
	uiString errmsg;
	const bool res =
		wd->d2TModel()->ensureValid( *wd, errmsg, &depths, &times );
	if ( !res )
	{
	    uiString msgtoadd;
	    msgtoadd.append( wd->name() ).append( " : " ).append( errmsg );
	    errmsgs.add( errmsg );
	    continue;
	}

	Well::Writer wtr( selwells[idx], *wd );
	wtr.putD2T();
    }

    if ( !errmsgs.isEmpty() )
    {
	uiMSG().errorWithDetails( errmsgs );
	return errmsgs.size() != selwells.size();
    }

    return true;
}
