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
#include "uicombobox.h"
#include "uid2tmodelgrp.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uitable.h"
#include "uitblimpexpdatasel.h"
#include "uiwellpartserv.h"

#include "ctxtioobj.h"
#include "file.h"
#include "iodir.h"
#include "ioobj.h"
#include "ioman.h"
#include "iodirentry.h"
#include "iopar.h"
#include "oddirs.h"
#include "randcolor.h"
#include "od_iostream.h"
#include "survinfo.h"
#include "tabledef.h"
#include "unitofmeasure.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "wellimpasc.h"
#include "welllog.h"
#include "welltransl.h"
#include "welllogset.h"
#include "welltrack.h"


static const char* trackcollbls[] = { "X", "Y", "Z", "MD", 0 };
static const int nremptyrows = 5;

uiWellTrackDlg::uiWellTrackDlg( uiParent* p, Well::Data& d )
	: uiDialog(p,uiDialog::Setup("Well Track",
				     "Edit Well Track",
				     "107.1.7"))
	, wd_(d)
	, track_(d.track())
	, orgtrack_(new Well::Track(d.track()))
	, fd_( *Well::TrackAscIO::getDesc() )

{
    tbl_ = new uiTable( this, uiTable::Setup().rowdesc("Point")
					      .rowgrow(true)
					      .defrowlbl("")
					      .removeselallowed(false),
		        "Well Track Table" );
    tbl_->setColumnLabels( trackcollbls );
    tbl_->setNrRows( nremptyrows );
    tbl_->setPrefWidth( 500 );
    tbl_->setPrefHeight( 400 );

    zinftfld_ = new uiCheckBox( this, "Z in Feet" );
    zinftfld_->setChecked( SI().depthsInFeet() );
    zinftfld_->activated.notify( mCB( this,uiWellTrackDlg,fillTable ) );
    zinftfld_->attach( ensureBelow, tbl_ );
    zinftfld_->attach( rightBorder );

    uiGroup* actbutgrp = new uiGroup( this, "Action buttons grp" );
    uiButton* updnowbut = new uiPushButton( actbutgrp, "&Update display",
					    mCB(this,uiWellTrackDlg,updNow),
					    true );
    uiButton* readbut = new uiPushButton( actbutgrp, "&Read new",
					    mCB(this,uiWellTrackDlg,readNew),
					    false );
    readbut->attach( rightOf, updnowbut );

    uiButton* expbut = new uiPushButton( actbutgrp, "&Export",
					 mCB(this,uiWellTrackDlg,exportCB),
					 false );
    expbut->attach( rightOf, readbut );
    actbutgrp->attach( centeredBelow, tbl_ );
    zinftfld_->attach( ensureRightOf, actbutgrp );

    fillTable();
}


bool uiWellTrackDlg::fillTable( CallBacker* )
{
    RowCol curcell( tbl_->currentCell() );

    const int sz = track_.nrPoints();
    int newsz = sz + nremptyrows;
    if ( newsz < 8 ) newsz = 8;
    tbl_->setNrRows( newsz );
    tbl_->clearTable();
    if ( !sz ) return false;

    const bool zinft = zinftfld_->isChecked();

    float fac = 1;
    if ( SI().zIsTime() )
	fac = zinft ? mToFeetFactorF : 1;
    else
    {
	if ( SI().zInFeet() && !zinft )
	    fac = mFromFeetFactorF;
	else if ( SI().zInMeter() && zinft )
	    fac = mToFeetFactorF;
    }

    for ( int idx=0; idx<sz; idx++ )
    {
	const Coord3& c( track_.pos(idx) );
	tbl_->setValue( RowCol(idx,0), c.x );
	tbl_->setValue( RowCol(idx,1), c.y );
	tbl_->setValue( RowCol(idx,2), (float)c.z*fac );
	tbl_->setValue( RowCol(idx,3), track_.dah(idx)*fac );
    }

    if ( curcell.row() >= newsz ) curcell.row() = newsz-1;
    tbl_->setCurrentCell( curcell );
    return true;
}


uiWellTrackDlg::~uiWellTrackDlg()
{
    delete orgtrack_;
    delete &fd_;
}


class uiWellTrackReadDlg : public uiDialog
{
public:

uiWellTrackReadDlg( uiParent* p, Table::FormatDesc& fd, Well::Track& track )
	: uiDialog(p,uiDialog::Setup("Read new Well Track",
				     "Specify new Well Track","107.1.8"))
	, track_(track)
{
    wtinfld_ = new uiFileInput( this, "Well Track File",
    uiFileInput::Setup().withexamine(true) );

    uiTableImpDataSel* dataselfld = new uiTableImpDataSel( this, fd, 0 );
    dataselfld->attach( alignedBelow, wtinfld_ );
}


bool acceptOK( CallBacker* )
{
    track_.setEmpty();
    fnm_ = wtinfld_->fileName();
    if ( File::isEmpty(fnm_.buf()) )
	{ uiMSG().error( "Invalid input file" ); return false; }
    return true;
}

    uiFileInput*	wtinfld_;
    BufferString	fnm_;
    Well::Track&        track_;
};


void uiWellTrackDlg::readNew( CallBacker* )
{
    uiWellTrackReadDlg dlg( this, fd_, track_ );
    if ( !dlg.go() ) return;

    if ( !dlg.fnm_.isEmpty() )
    {
	od_istream strm( dlg.fnm_ );
	if ( !strm.isOK() )
	    { uiMSG().error( "Cannot open input file" ); return; }

	Well::TrackAscIO wellascio(fd_, strm );
	if ( !wellascio.getData( wd_, true ) )
	    uiMSG().error( "Failed to convert into compatible data" );

	tbl_->clearTable();
	if ( !fillTable() )
	    return;

	wd_.trackchanged.trigger();
    }
    else
	uiMSG().error( "Please select a file" );
}


bool uiWellTrackDlg::updNow( CallBacker* )
{
    track_.setEmpty();
    const int nrrows = tbl_->nrRows();
    const bool zinft = zinftfld_->isChecked();

    float fac = 1;
    if ( SI().zIsTime() && zinft )
	fac = mFromFeetFactorF;
    else if ( SI().zInFeet() && !zinft )
	fac = mToFeetFactorF;
    else if ( SI().zInMeter() && zinft )
	fac = mFromFeetFactorF;

    bool needfill = false;
    for ( int idx=0; idx<nrrows; idx++ )
    {
	const char* sval = tbl_->text( RowCol(idx,0) );
	if ( !*sval ) continue;
	const double xval = toDouble(sval);
	sval = tbl_->text( RowCol(idx,1) );
	if ( !*sval ) continue;
	const double yval = toDouble(sval);
	sval = tbl_->text( RowCol(idx,2) );
	if ( !*sval ) continue;
	const double zval = toDouble(sval) * fac;

	const Coord3 newc( xval, yval, zval );
	if ( !SI().isReasonable( newc ) )
	{
	    BufferString msg( "Found undefined values in row ", idx+1, "." );
	    msg.add( "Please enter valid values" );
	    uiMSG().message( msg );
	    return false;
	}

	sval = tbl_->text( RowCol(idx,3) );
	float dahval = 0;
	if ( *sval )
	    dahval = toFloat(sval) * fac;
	else if ( idx > 0 )
	{
	    dahval = (float) ( track_.dah(idx-1) +
					track_.pos(idx-1).distTo( newc ) );
	    needfill = true;
	}

	track_.addPoint( newc, (float) newc.z, dahval );
    }

    if ( track_.nrPoints() > 1 )
    {
	wd_.info().surfacecoord = track_.pos(0);
	wd_.trackchanged.trigger();
    }
    else
    {
	uiMSG().error( "Please define at least two points." );
	return false;
    }

    if ( needfill && !fillTable() )
	return false;

    return true;
}


bool uiWellTrackDlg::rejectOK( CallBacker* )
{
    track_ = *orgtrack_;
    wd_.trackchanged.trigger();
    return true;
}


bool uiWellTrackDlg::acceptOK( CallBacker* )
{
    if ( !updNow( 0 ) )
	return false;

    const int nrpts = track_.nrPoints();
    if ( nrpts < 2 ) return false;
    const int orgnrpts = orgtrack_->nrPoints();
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
	BufferString msg( "You have changed at least one MD value.\nMarkers" );
	if ( SI().zIsTime() )
	    msg += ", logs, T/D and checkshot models";
	else
	    msg += " and logs";
	msg += " are based on the old MD values.\n"
	       "They may therefore become invalid.\n\nContinue?";
	if ( !uiMSG().askGoOn(msg) )
	    return false;
    }

    return true;
}


void uiWellTrackDlg::exportCB( CallBacker* )
{
    if ( !track_.size() )
    {
	uiMSG().message( "No data available to export" );
	return;
    }

    uiFileDialog fdlg( this, false, 0, 0, "File name for export" );
    fdlg.setDirectory( GetDataDir() );
    if ( !fdlg.go() )
	return;

    od_ostream strm( fdlg.fileName() );
    if ( !strm.isOK() )
    {
	uiMSG().error( BufferString( "Cannot open '", fdlg.fileName(),
				     "' for write" ) );
	return;
    }

    for ( int idx=0; idx<track_.size(); idx++ )
    {
	const Coord3 coord( track_.pos(idx) );
	strm << coord.x << od_tab << coord.y << od_tab << coord.z
	     << od_tab << track_.dah(idx) << od_newline;
    }
}


// ==================================================================


static const char* sKeyMD()		{ return "MD"; }
static const char* sKeyTVD()		{ return "TVD"; }
static const char* sKeyTVDGL()		{ return "TVDGL"; }
static const char* sKeyTVDSD()		{ return "TVDSD"; }
static const char* sKeyTVDSS()		{ return "TVDSS"; }
static const char* sKeyTWT()		{ return "TWT"; }
static const char* sKeyOWT()		{ return "OWT"; }
static const char* sKeyVint()		{ return "Vint"; }
static const int cMDCol = 0;
static const int cTVDCol = 1;

#define mD2TModel (cksh_ ? wd_.checkShotModel() : wd_.d2TModel())

uiD2TModelDlg::uiD2TModelDlg( uiParent* p, Well::Data& wd, bool cksh )
	: uiDialog(p,uiDialog::Setup("Depth/Time Model",
		 BufferString("Edit ",cksh?"Checkshot":"Time/Depth"," model"),
				     "107.1.5"))
	, wd_(wd)
	, cksh_(cksh)
	, orgd2t_(mD2TModel ? new Well::D2TModel(*mD2TModel) : 0)
        , tbl_(0)
        , unitfld_(0)
        , timefld_(0)
{
    tbl_ = new uiTable( this, uiTable::Setup()
				.rowdesc(cksh_ ? "Measure point" : "Control Pt")
				.rowgrow(true)
				.defrowlbl("")
				.selmode(uiTable::Single)
				.removeselallowed(false),
		  BufferString( cksh_ ? "CheckShot" : "Time-Depth", " model" ));
    BufferStringSet header;
    getColLabels( header );
    tbl_->setColumnLabels( header );
    tbl_->setNrRows( nremptyrows );
    tbl_->valueChanged.notify( mCB(this,uiD2TModelDlg,dtpointChangedCB) );
    tbl_->rowDeleted.notify( mCB(this,uiD2TModelDlg,dtpointRemovedCB) );
    tbl_->setColumnToolTip( cMDCol,
	   "Measured depth along the borehole, origin at Reference Datum" );
    tbl_->setColumnToolTip( cTVDCol,
	    "True Vertical Depth, origin at Reference Datum" );
    tbl_->setColumnToolTip( getTVDSSCol(),
	    "True Vertical Depth Sub-Sea, positive downwards" );
    tbl_->setColumnToolTip( getTimeCol(),
	    "Two-way travel times with same origin as seismic survey: SRD=0" );
    tbl_->setColumnToolTip( getVintCol(),
	    "Interval velocity above this control point (read-only)" );
    tbl_->setPrefWidth( 700 );

    timefld_ = new uiCheckBox( this, " Time is TWT" );
    timefld_->setChecked( true );
    timefld_->activated.notify( mCB(this,uiD2TModelDlg,fillTable) );
    timefld_->attach( rightAlignedBelow, tbl_ );

    unitfld_ = new uiCheckBox( this, " Z in feet" );
    unitfld_->setChecked( SI().depthsInFeet() );
    unitfld_->activated.notify( mCB(this,uiD2TModelDlg,fillTable) );
    unitfld_->attach( rightAlignedBelow, timefld_ );

    uiGroup* actbutgrp = new uiButtonGroup( this, "Action buttons", false );
    if ( !cksh_ )
	new uiPushButton( actbutgrp, "&Update display",
			  mCB(this,uiD2TModelDlg,updNow), true );
    actbutgrp->attach( leftAlignedBelow, tbl_ );

    uiGroup* iobutgrp = new uiButtonGroup( this, "Input/output buttons", false);
    new uiPushButton( iobutgrp, "&Read new", mCB(this,uiD2TModelDlg,readNew),
		      false );
    new uiPushButton( iobutgrp, "&Export", mCB(this,uiD2TModelDlg,expData),
		      false );
    iobutgrp->attach( leftTo, unitfld_ );

    fillTable(0);
}


void uiD2TModelDlg::getColLabels( BufferStringSet& lbls ) const
{
    bool zinfeet = false;
    if ( unitfld_ )
	zinfeet = unitfld_->isChecked();

    bool timeisoneway = false;
    if ( timefld_ )
	timeisoneway = !timefld_->isChecked();

    BufferString curlbl;

    curlbl = sKeyMD();
    curlbl.add( "(" ).add( getDistUnitString(zinfeet,false) ).add( ")" );
    lbls.add( curlbl );
    curlbl = sKeyTVD();
    curlbl.add( "(" ).add( getDistUnitString(zinfeet,false) ).add( ")" );
    lbls.add( curlbl );

    if ( !mIsUdf(getTVDGLCol()) )
    {
	curlbl = sKeyTVDGL();
	curlbl.add( "(" ).add( getDistUnitString(zinfeet,false) ).add( ")" );
	lbls.add( curlbl );
    }

    curlbl = sKeyTVDSS();
    curlbl.add( "(" ).add( getDistUnitString(zinfeet,false) ).add( ")" );
    lbls.add( curlbl );

    if ( !mIsUdf(getTVDSDCol()) )
    {
	curlbl = sKeyTVDSD();
	curlbl.add( "(" ).add( getDistUnitString(zinfeet,false) ).add( ")" );
	lbls.add( curlbl );
    }

    curlbl = timeisoneway ? sKeyOWT() : sKeyTWT();
    curlbl.add( "(ms)" );
    lbls.add( curlbl );

    curlbl = sKeyVint();
    curlbl.add( "(" ).add( getDistUnitString(zinfeet,false) ).add( "/s)" );
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

#define mGetVel(dah,d2t) \
{ \
    wd_.track().getPos(dah).z < 1e-2f - SI().seismicReferenceDatum() && \
    !mIsUdf(wd_.info().replvel)\
	   ? wd_.info().replvel \
	   : mCast(float,d2t->getVelocityForDah( dah, wd_.track() )) \
}


void uiD2TModelDlg::fillTable( CallBacker* )
{
    NotifyStopper ns( tbl_->valueChanged );
    tbl_->setColumnReadOnly( getVintCol(), false );
    const Well::D2TModel* d2t = mD2TModel;
    const Well::Track& track = wd_.track();
    const int tracksz = wd_.track().nrPoints();
    if ( !d2t || d2t->size()<2 )
	return;

    if ( tracksz<2 )
    {
	uiMSG().error( "Invalid track" );
	return;
    }

    const int dtsz = d2t->size();
    tbl_->setNrRows( dtsz + nremptyrows );
    BufferStringSet header;
    getColLabels( header );
    tbl_->setColumnLabels( header );

    const float zfac = !unitfld_->isChecked() ? 1 : mToFeetFactorF;
    const float twtfac = timefld_->isChecked() ? 1000.f : 500.f;
    const float kbelev = wd_.track().getKbElev();
    const float groundevel = wd_.info().groundelev;
    const float srd = mCast(float,SI().seismicReferenceDatum());
    const bool hastvdgl = !mIsUdf( groundevel );
    const bool hastvdsd = !mIsZero( srd, 1e-3f );
    for ( int idx=0; idx<dtsz; idx++ )
    {
	const float dah = d2t->dah(idx);
	const float tvdss = mCast(float,track.getPos(dah).z);
	const float tvd = tvdss + kbelev;
	const float vint = mGetVel(dah,d2t);
	tbl_->setValue( RowCol(idx,cMDCol), dah * zfac );
	tbl_->setValue( RowCol(idx,cTVDCol), tvd * zfac );
	if ( hastvdgl )
	{
	    const float tvdgl = ( tvdss + groundevel ) * zfac;
	    tbl_->setValue( RowCol(idx,getTVDGLCol()), tvdgl );
	}

	if ( hastvdsd )
	{
	    const float tvdsd = ( tvdss + srd ) * zfac;
	    tbl_->setValue( RowCol(idx,getTVDSDCol()), tvdsd );
	}

	tbl_->setValue( RowCol(idx,getTVDSSCol()), tvdss * zfac );
	tbl_->setValue( RowCol(idx,getTimeCol()), d2t->t(idx) * twtfac );
	tbl_->setValue( RowCol(idx,getVintCol()), vint * zfac );
    }
    tbl_->setColumnReadOnly( getVintCol(), true );
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
	uiMSG().error("Invalid time-depth model");
	return;
    }

    const int row = tbl_->currentRow();
    const float zfac = !unitfld_->isChecked() ? 1.f : mToFeetFactorF;
    int idah = d2t->indexOf( tbl_->getfValue(RowCol(row,cMDCol)) / zfac );
    if ( ( rowIsIncomplete(row) && !mIsUdf(getNextCompleteRowIdx(row)) ) ||
	 mIsUdf(idah) )
	return;

    d2t->remove( idah-1 );
    const int nextrow = getNextCompleteRowIdx( row-1 );
    if ( mIsUdf(nextrow) )
	return;

    idah = d2t->indexOf( tbl_->getfValue(RowCol(nextrow,cMDCol)) / zfac );
    const float olddah = d2t->dah( idah );
    updateDtpoint( nextrow, olddah );
}


#define mErrRet(msg) { uiMSG().error(msg); return false; }
bool uiD2TModelDlg::updateDtpointDepth( int row )
{
    NotifyStopper ns( tbl_->valueChanged );
    const Well::D2TModel* d2t = mD2TModel;
    const Well::Track& track = wd_.track();
    const int tracksz = wd_.track().nrPoints();
    if ( !d2t || d2t->size()<2 || tracksz<2 )
    {
	BufferString errmsg = tracksz<2 ? "Invalid track"
					: "Invalid time-depth model";
	mErrRet(errmsg)
    }

    const float zfac = !unitfld_->isChecked() ? 1.f : mToFeetFactorF;
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
    if ( mIsUdf(tbl_->getfValue(rcin)) )
    {
	uiMSG().error( "Please enter a valid number" );
	if ( !newrow )
	    tbl_->setValue( rcin, oldval * zfac );

	return false;
    }

    float inval = tbl_->getfValue( rcin ) / zfac;
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
	       : tbl_->getfValue( RowCol(prevrow,incol) ) / zfac;
    tblrg.stop = d2t->size() < row || mIsUdf(nextrow) ? zrange.stop
	       : tbl_->getfValue( RowCol(nextrow,incol) ) / zfac;

    BufferString lbl;
    if ( md2tvd ) lbl = sKeyMD();
    else if ( inistvd ) lbl =  sKeyTVD();
    else if ( inistvdgl ) lbl = sKeyTVDGL();
    else if ( inistvdsd ) lbl = sKeyTVDSD();
    else if ( inistvdss ) lbl = sKeyTVDSS();

    BufferString errmsg = "The entered ";
    if ( !zrange.includes(inval,true) )
    {
	errmsg.add( lbl ).add( " value " ).add( inval * zfac );
	errmsg.add( " is outside of track range\n" );
	errmsg.add( "[" ).add( zrange.start * zfac );
	errmsg.add( ", " ).add( zrange.stop * zfac ).add( "]" );
	errmsg.add( getDistUnitString(unitfld_->isChecked(),false) );
	errmsg.add( " (" ).add( lbl ).add( ")" );
	tbl_->setValue( rcin, !newrow ? oldval * zfac : mUdf(float) );
	mErrRet(errmsg)
    }

    if ( !tblrg.includes(inval,true) )
    {
	errmsg.add( lbl ).add( " is not between " );
	errmsg.add( "the depths of the previous and next control points" );
	tbl_->setValue( rcin, !newrow ? oldval * zfac : mUdf(float) );
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
	tbl_->setValue( RowCol(row,cMDCol), dah * zfac );

    if ( !inistvdss )
	tbl_->setValue( RowCol(row,getTVDSSCol()), tvdss * zfac );

    if ( !inistvd )
	tbl_->setValue( RowCol(row,cTVDCol), ( tvdss + kbelev ) * zfac );

    if ( hastvdgl && !inistvdgl )
	tbl_->setValue( RowCol(row,getTVDGLCol()), ( tvdss + groundevel )*zfac);

    if ( hastvdsd && !inistvdsd )
	tbl_->setValue( RowCol(row,getTVDSDCol()), ( tvdss + srd ) * zfac );

    const float twtfac = timefld_->isChecked() ? 2000.f : 1000.f;
    Interval<float> replvelint( track.getKbElev(), srd );
    if ( replvelint.includes(-1.f*tvdss,true) && !mIsUdf(wd_.info().replvel) )
    {
	const float twt = twtfac * (tvdss + srd ) / wd_.info().replvel;
	tbl_->setValue( RowCol(row,getTimeCol() ), twt );
    }

    return updateDtpoint( row, olddah );
}


bool uiD2TModelDlg::updateDtpointTime( int row )
{
    NotifyStopper ns( tbl_->valueChanged );
    const Well::D2TModel* d2t = mD2TModel;
    const Well::Track& track = wd_.track();
    const int tracksz = wd_.track().nrPoints();
    if ( !d2t || d2t->size()<2 || tracksz<2 )
    {
	BufferString errmsg = tracksz<2 ? "Invalid track"
					: "Invalid time-depth model";
	mErrRet(errmsg)
    }

    const RowCol rcin(row,getTimeCol());
    const bool newrow = rowIsIncomplete( row );
    const float oldval = newrow ? mUdf(float) : d2t->value( row );
    const float twtfac = timefld_->isChecked() ? 1000.f : 500.f;
    if ( mIsUdf(tbl_->getfValue(rcin)) )
    {
	uiMSG().error( "Please enter a valid number" );
	if ( !newrow )
	    tbl_->setValue( rcin, oldval * twtfac );

	return false;
    }

    const float inval = tbl_->getfValue( rcin ) / twtfac;
    Interval<float> dahrg = track.dahRange();
    Interval<float> timerg;
    timerg.start = d2t->getTime( dahrg.start, track );
    timerg.stop = d2t->getTime( dahrg.stop, track );
    Interval<float> tblrg;
    const int prevrow = getPreviousCompleteRowIdx( row );
    const int nextrow = getNextCompleteRowIdx( row );
    tblrg.start = row == 0 || mIsUdf(prevrow) ? timerg.start
	       : tbl_->getfValue( RowCol(prevrow,getTimeCol()) ) / twtfac;
    tblrg.stop = d2t->size() < row || mIsUdf(nextrow) ? timerg.stop
	       : tbl_->getfValue( RowCol(nextrow,getTimeCol()) ) / twtfac;

    if ( !tblrg.includes(inval,true) &&
	 !mIsUdf(getPreviousCompleteRowIdx(row)) &&
	 !mIsUdf(getNextCompleteRowIdx(row)) )
    {
	BufferString errmsg( "The entered time is not between " );
	errmsg.add( "the times of the previous and next control points" );
	tbl_->setValue( rcin, !newrow ? oldval * twtfac : mUdf(float) );
	mErrRet(errmsg)
    }

    if ( inval < 0.f && !mIsUdf(wd_.info().replvel) )
    {
	const float zfac = !unitfld_->isChecked() ? 1.f : mToFeetFactorF;
	const float tvdss = ( wd_.info().replvel * inval * 500.f / twtfac
			  - mCast(float, SI().seismicReferenceDatum()) ) * zfac;
	const RowCol rc( row, getTVDSSCol() );
	tbl_->setValue( rc, tvdss );
	tbl_->setSelected( rc );
	updateDtpointDepth( row );
    }

    return updateDtpoint( row, oldval );
}


bool uiD2TModelDlg::updateDtpoint( int row, float oldval )
{
    NotifyStopper ns( tbl_->valueChanged );
    tbl_->setColumnReadOnly( getVintCol(), false );
    Well::D2TModel* d2t = mD2TModel;
    const Well::Track& track = wd_.track();
    const int tracksz = wd_.track().nrPoints();
    if ( !d2t || d2t->size()<2 || tracksz<2 )
    {
	BufferString errmsg = tracksz<2 ? "Invalid track"
					: "Invalid time-depth model";
	mErrRet(errmsg)
    }

    const RowCol rcdah(row,cMDCol);
    const RowCol rctwt(row,getTimeCol());
    if ( mIsUdf(tbl_->getfValue(rcdah)) || mIsUdf(tbl_->getfValue(rctwt)) )
	return true; // not enough data yet to proceed

    if ( !rowIsIncomplete(row) )
    {
	const bool oldvalisdah = tbl_->currentCol() < getTimeCol();
	const float olddah = oldvalisdah ? oldval : d2t->getDah( oldval, track);
	const int dahidx = d2t->indexOf( olddah );
	d2t->remove( dahidx );
    }

    const float zfac = !unitfld_->isChecked() ? 1.f : mToFeetFactorF;
    const float twtfac = timefld_->isChecked() ? 1000.f : 500.f;
    const float dah = tbl_->getfValue( rcdah ) / zfac;
    const float twt = tbl_->getfValue( rctwt ) / twtfac;
    d2t->insertAtDah( dah, twt );
    wd_.d2tchanged.trigger();

    const float newvint = mGetVel(dah,d2t);
    const RowCol rcvint(row,getVintCol());
    tbl_->setValue( rcvint, newvint );

    for ( int irow=row+1; irow<tbl_->nrRows(); irow++ )
    {
	if ( rowIsIncomplete(irow) )
	    continue;

	const float nextdah = tbl_->getfValue( RowCol(irow,cMDCol) ) / zfac;
	const float nextvint = mGetVel(nextdah,d2t);
	tbl_->setValue( RowCol(irow,getVintCol()), nextvint * zfac );
	break;
    }

    tbl_->setColumnReadOnly( getVintCol(), true );
    return true;
}


bool uiD2TModelDlg::rowIsIncomplete( int row ) const
{
    Well::D2TModel* d2t = mD2TModel;
    if ( !d2t || d2t->size()<2 )
	mErrRet( "Invalid time-depth model" )

    if ( row >= d2t->size() )
	return true;

    return mIsUdf( tbl_->getfValue( RowCol(row,getVintCol()) ) );
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
{
public:

uiD2TModelReadDlg( uiParent* p, Well::Data& wd, bool cksh )
	: uiDialog(p,uiDialog::Setup("Read new data",
				     "Specify input file","107.1.6"))
	, cksh_(cksh)
	, wd_(wd)
{
    uiD2TModelGroup::Setup su( false );
    su.filefldlbl( "File name" );
    d2tgrp = new uiD2TModelGroup( this, su );
}


bool acceptOK( CallBacker* )
{
    const char* errmsg = d2tgrp->getD2T( wd_, cksh_ );
    if ( errmsg && *errmsg )
	{ uiMSG().error( errmsg ); return false; }

    if ( wd_.d2TModel() )
	wd_.d2TModel()->deInterpolate();

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

    const BufferString errmsg = dlg.d2tgrp->getD2T( wd_, cksh_ );
    if ( !errmsg.isEmpty() )
	uiMSG().error( errmsg );
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
    if ( d2t->size() < 2 )
	{ uiMSG().error( "No valid data entered" ); return; }

    uiFileDialog dlg( this, false, 0, 0, "Filename for export" );
    dlg.setDirectory( GetDataDir() );
    if ( !dlg.go() )
	return;

    const BufferString fnm( dlg.fileName() );
    od_ostream strm( fnm );
    if ( !strm.isOK() )
    {
	uiMSG().error( BufferString("Cannot open '", fnm, "' for write") );
	return;
    }

    const float zfac = !unitfld_->isChecked() ? 1 : mToFeetFactorF;
    const float twtfac = timefld_->isChecked() ? 1000.f : 500.f;
    const float kbelev = wd_.track().getKbElev();
    const float groundevel = wd_.info().groundelev;
    const float srd = mCast(float,SI().seismicReferenceDatum());
    const bool hastvdgl = !mIsUdf( groundevel );
    const bool hastvdsd = !mIsZero( srd, 1e-3f );
    BufferStringSet header;
    getColLabels( header );

    strm << header.get( cMDCol ) << od_tab <<  header.get( cTVDCol ) << od_tab;
    if ( hastvdgl )
	strm << header.get( getTVDGLCol() ) << od_tab;
    if ( hastvdsd )
	strm << header.get( getTVDSDCol() ) << od_tab;
    strm << header.get( getTVDSSCol() ) << od_tab;
    strm << header.get( getTimeCol() ) << od_tab;
    strm << header.get( getVintCol() ) << od_newline;
    for ( int idx=0; idx<d2t->size(); idx++ )
    {
	const float dah = d2t->dah(idx) * zfac;
	const float tvdss = mCast(float,wd_.track().getPos(dah).z) * zfac;
	const float tvd = tvdss + kbelev * zfac;
	const float twt = d2t->t(idx) * twtfac;
	const float vint = mGetVel(dah,d2t);
	strm << dah << od_tab << tvd << od_tab;
	if ( hastvdgl )
	{
	    const float tvdgl = tvdss + groundevel * zfac;
	    strm << tvdgl << od_tab;
	}
	if ( hastvdsd )
	{
	    const float tvdsd = tvdss + srd * zfac;
	    strm << tvdsd << od_tab;
	}
	strm << tvdss << od_tab << twt << od_tab << vint * zfac << od_newline;
    }
}


bool uiD2TModelDlg::getFromScreen()
{
    Well::D2TModel* d2t = mD2TModel;
    getModel( *d2t );

    if ( d2t->size() < 2 )
	mErrRet( "Please define at least two control points." )

    wd_.d2tchanged.trigger();
    return true;
}


void uiD2TModelDlg::updNow( CallBacker* )
{
    getFromScreen();
}


void uiD2TModelDlg::getModel( Well::D2TModel& d2t )
{
    d2t.setEmpty();
    const float zfac = !unitfld_->isChecked() ? 1 : mToFeetFactorF;
    const float twtfac = timefld_->isChecked() ? 1000.f : 500.f;
    const int nrrows = tbl_->nrRows();
    for ( int irow=0; irow<nrrows; irow++ )
    {
	if ( mIsUdf(tbl_->getfValue( RowCol(irow,getVintCol()))) )
	    continue;

	const float dah = tbl_->getfValue( RowCol(irow,cMDCol) ) / zfac;
	const float twt = tbl_->getfValue( RowCol(irow,getTimeCol()) ) / twtfac;
	d2t.add( dah, twt );
    }
}


bool uiD2TModelDlg::rejectOK( CallBacker* )
{
    Well::D2TModel* d2t = mD2TModel;
    if ( d2t )
	*d2t = *orgd2t_;

    wd_.d2tchanged.trigger();
    return true;
}


bool uiD2TModelDlg::acceptOK( CallBacker* )
{
    return getFromScreen();
}



// ==================================================================

static const float defundefval = -999.25;
#ifdef __win__
    static const char* lasfileflt = "Las files (*.las *.dat)";
#else
    static const char* lasfileflt = "Las files (*.las *.LAS *.dat *.DAT)";
#endif


uiLoadLogsDlg::uiLoadLogsDlg( uiParent* p, Well::Data& wd_ )
    : uiDialog(p,uiDialog::Setup("Logs","Define log parameters","107.1.2"))
    , wd(wd_)
{
    lasfld = new uiFileInput( this, "Input (pseudo-)LAS logs file",
			      uiFileInput::Setup(uiFileDialog::Gen)
			      .filter(lasfileflt).withexamine(true) );
    lasfld->valuechanged.notify( mCB(this,uiLoadLogsDlg,lasSel) );

    intvfld = new uiGenInput( this, "Depth interval to load (empty=all)",
			      FloatInpIntervalSpec(false) );
    intvfld->attach( alignedBelow, lasfld );
    BoolInpSpec mft( !SI().depthsInFeet(), "Meter", "Feet" );
    intvunfld = new uiGenInput( this, "", mft );
    intvunfld->attach( rightOf, intvfld );
    intvunfld->display( false );

    unitlbl = new uiLabel( this, "XXXX" );
    unitlbl->attach( rightOf, intvfld );
    unitlbl->display( false );

    istvdfld = new uiGenInput( this, "Depth values are",
				BoolInpSpec(false,"TVDSS","MD") );
    istvdfld->attach( alignedBelow, intvfld );

    udffld = new uiGenInput( this, "Undefined value in logs",
                    FloatInpSpec(defundefval));
    udffld->attach( alignedBelow, istvdfld );

    logsfld = new uiLabeledListBox( this, "Select logs", true );
    logsfld->attach( alignedBelow, udffld );
}


void uiLoadLogsDlg::lasSel( CallBacker* )
{
    const char* lasfnm = lasfld->text();
    if ( !lasfnm || !*lasfnm ) return;

    Well::Data wd_; Well::LASImporter wdai( wd_ );
    Well::LASImporter::FileInfo lfi;
    const char* res = wdai.getLogInfo( lasfnm, lfi );
    if ( res ) { uiMSG().error( res ); return; }

    logsfld->box()->setEmpty();
    logsfld->box()->addItems( lfi.lognms );
    logsfld->box()->selectAll( true );

    BufferString lbl( "(" ); lbl += lfi.zunitstr.buf(); lbl += ")";
    unitlbl->setText( lbl );
    unitlbl->display( true );
    bool isft = *lfi.zunitstr.buf() == 'f' || *lfi.zunitstr.buf() == 'F';
    intvunfld->setValue( !isft );
    intvunfld->display( false );

    udffld->setValue( lfi.undefval );

    Interval<float> usrzrg = lfi.zrg;
    if ( isft )
    {
	if ( !mIsUdf(lfi.zrg.start) )
	    usrzrg.start *= mToFeetFactorF;
	if ( !mIsUdf(lfi.zrg.stop) )
	    usrzrg.stop *= mToFeetFactorF;
    }
    intvfld->setValue( usrzrg );
}


bool uiLoadLogsDlg::acceptOK( CallBacker* )
{
    Well::LASImporter wdai( wd );
    Well::LASImporter::FileInfo lfi;

    lfi.undefval = udffld->getfValue();

    Interval<float> usrzrg = intvfld->getFInterval();
    const bool zinft = !intvunfld->getBoolValue();
    if ( zinft )
    {
	if ( !mIsUdf(usrzrg.start) )
	    usrzrg.start *= mFromFeetFactorF;
	if ( !mIsUdf(usrzrg.stop) )
	    usrzrg.stop *= mFromFeetFactorF;
    }
    lfi.zrg.setFrom( usrzrg );

    const char* lasfnm = lasfld->text();
    if ( !lasfnm || !*lasfnm )
	{ uiMSG().error("Enter valid filename"); return false; }

    BufferStringSet lognms;
    for ( int idx=0; idx<logsfld->box()->size(); idx++ )
    {
	if ( logsfld->box()->isSelected(idx) )
	    lognms += new BufferString( logsfld->box()->textOfItem(idx) );
    }

    BufferString existlogmsg;
    for ( int idx=lognms.size()-1; idx>=0; idx-- )
    {
	if ( wd.logs().getLog( lognms.get( idx ) ) )
	{
	    if ( !existlogmsg.isEmpty() ) existlogmsg += ", ";
	    existlogmsg += lognms.get( idx );
	    lognms.removeSingle( idx );
	}
    }
    if ( !existlogmsg.isEmpty() )
    {
	existlogmsg.add( " already exist(s) and will not be loaded.\n\n" );
	existlogmsg.add( "Please remove them from " );
	existlogmsg.add( "the existing logs before import." );
	uiMSG().warning( existlogmsg );
    }
    else if ( lognms.isEmpty() )
	{ uiMSG().error("Please select at least one log"); return false; }


    lfi.lognms = lognms;
    const char* res = wdai.getLogs( lasfnm, lfi, istvdfld->getBoolValue() );
    if ( res ) { uiMSG().error( res ); return false; }

    return true;
}


// ==================================================================


static const char* exptypes[] =
{
    "MD/Value",
    "XYZ/Value",
    "ICZ/Value",
    0
};
static const float cTWTFac  = 1000.f;


uiExportLogs::uiExportLogs( uiParent* p, const ObjectSet<Well::Data>& wds,
			  const BufferStringSet& logsel )
    : uiDialog(p,uiDialog::Setup("Export Well logs",
				 "Specify format","107.1.3"))
    , wds_(wds)
    , logsel_(logsel)
    , multiwellsnamefld_(0)
{
    const bool zinft = SI().depthsInFeet();
    BufferString lbl( "Depth range " ); lbl += zinft ? "(ft)" : "(m)";
    zrangefld_ = new uiGenInput( this, lbl, FloatInpIntervalSpec(true) );
    setDefaultRange( zinft );

    typefld_ = new uiGenInput( this, "Output format",
			      StringListInpSpec(exptypes) );
    typefld_->valuechanged.notify( mCB(this,uiExportLogs,typeSel) );
    typefld_->attach( alignedBelow, zrangefld_ );

    zunitgrp_ = new uiButtonGroup( this, "", false );
    zunitgrp_->attach( alignedBelow, typefld_ );
    uiLabel* zlbl = new uiLabel( this, "Output Z-unit" );
    zlbl->attach( leftOf, zunitgrp_ );
    new uiRadioButton( zunitgrp_, "meter" );
    new uiRadioButton( zunitgrp_, "feet" );
    bool have2dtmodel = true;
    for ( int idwell=0; idwell<wds_.size(); idwell++ )
    {
	if ( !wds_[idwell]->haveD2TModel() )
	{ have2dtmodel = false; break; }
    }
    if ( SI().zIsTime() && have2dtmodel)
    {
	new uiRadioButton( zunitgrp_, "sec" );
	new uiRadioButton( zunitgrp_, "msec" );
    }
    zunitgrp_->selectButton( zinft );

    const bool multiwells = wds.size() > 1;
    outfld_ = new uiFileInput( this, multiwells ? "File Directory"
						: "Output file",
			      uiFileInput::Setup().forread(false)
						  .directories(multiwells) );
    outfld_->attach( alignedBelow, zunitgrp_ );
    if ( multiwells )
    {
	outfld_->setFileName( IOM().rootDir() );
	multiwellsnamefld_ = new uiGenInput( this, "File name suffix" );
	multiwellsnamefld_->attach( alignedBelow, outfld_ );
	multiwellsnamefld_->setText( "logs.txt" );
    }

    typeSel(0);
}


void uiExportLogs::setDefaultRange( bool zinft )
{
    StepInterval<float> dahintv;
    for ( int idwell=0; idwell<wds_.size(); idwell++ )
    {
	const Well::Data& wd = *wds_[idwell];
	for ( int idx=0; idx<wd.logs().size(); idx++ )
	{
	    const Well::Log& log = wd.logs().getLog(idx);
	    const int logsz = log.size();
	    if ( !logsz ) continue;

	    dahintv.include( wd.logs().dahInterval() );
	    const float width = log.dah(logsz-1) - log.dah(0);
	    dahintv.step = width / (logsz-1);
	    break;
	}
    }

    if ( zinft )
    {
	dahintv.start *= mToFeetFactorF;
	dahintv.stop *= mToFeetFactorF;
	dahintv.step *= mToFeetFactorF;
    }

    zrangefld_->setValue( dahintv );
}


void uiExportLogs::typeSel( CallBacker* )
{
    zunitgrp_->setSensitive( 2, typefld_->getIntValue() );
    zunitgrp_->setSensitive( 3, typefld_->getIntValue() );
}


bool uiExportLogs::acceptOK( CallBacker* )
{
    BufferString fname = outfld_->fileName();
    if ( fname.isEmpty() )
	 mErrRet( "Please select valid entry for the output" );

    BufferStringSet fnames;
    if ( wds_.size() > 1 )
    {
	if ( !File::isDirectory(fname) )
	    mErrRet( "Please enter a valid (existing) location" )
	BufferString suffix = multiwellsnamefld_->text();
	if ( suffix.isEmpty() )
	    mErrRet( "Please enter a valid file name" )

	for ( int idx=0; idx<wds_.size(); idx++ )
	{
	    BufferString nm( fname );
	    nm += "/"; nm += wds_[idx]->name(); nm += "_"; nm += suffix;
	    fnames.add( nm );
	}
    }
    else
	fnames.add( fname );

    for ( int idx=0; idx<fnames.size(); idx++ )
    {
	const BufferString fnm( fnames.get(idx) );
	od_ostream strm( fnm );
	if ( !strm.isOK() )
	{
	    BufferString msg( "Cannot open output file ", fnm );
	    strm.addErrMsgTo( msg );
	    mErrRet( msg );
	}
	writeHeader( strm, *wds_[idx] );
	writeLogs( strm, *wds_[idx] );
    }
    return true;
}


void uiExportLogs::writeHeader( od_ostream& strm, const Well::Data& wd )
{
    const char* units[] = { "(m)", "(ft)", "(s)", "(ms)", 0 };

    if ( typefld_->getIntValue() == 1 )
	strm << "X\tY\t";
    else if ( typefld_->getIntValue() == 2 )
	strm << "Inline\tCrossline\t";

    const int unitid = zunitgrp_->selectedId();
    BufferString zstr( unitid<2 ? "Depth" : "Time" );
    strm << zstr << units[unitid];

    for ( int idx=0; idx<wd.logs().size(); idx++ )
    {
	const Well::Log& log = wd.logs().getLog(idx);
	if ( !logsel_.isPresent( log.name() ) ) continue;
	BufferString lognm( log.name() );
	lognm.clean();
	lognm.replace( '+', '_' );
	lognm.replace( '-', '_' );
	strm << od_tab << lognm;
	if ( *log.unitMeasLabel() )
	    strm << "(" << log.unitMeasLabel() << ")";
    }

    strm << od_newline;
}


void uiExportLogs::writeLogs( od_ostream& strm, const Well::Data& wd )
{
    const bool infeet = zunitgrp_->selectedId() == 1;
    const bool insec = zunitgrp_->selectedId() == 2;
    const bool inmsec = zunitgrp_->selectedId() == 3;
    const bool intime = insec || inmsec;
    if ( intime && !wd.d2TModel() )
    {
	uiMSG().error( "No depth-time model found, cannot export with time" );
	return;
    }

    const bool zinft = SI().depthsInFeet();
    const int outtypesel = typefld_->getIntValue();
    const bool dobinid = outtypesel == 2;
    const StepInterval<float> intv = zrangefld_->getFStepInterval();
    const int nrsteps = intv.nrSteps();

    for ( int idx=0; idx<nrsteps; idx++ )
    {
	float md = intv.atIndex( idx );
	if ( zinft ) md *= mFromFeetFactorF;
	if ( outtypesel == 0 )
	{
	    const float mdout = infeet ? md*mToFeetFactorF : md;
	    strm << mdout;
	}
	else
	{
	    const Coord3 pos = wd.track().getPos( md );
	    if ( !pos.x && !pos.y && !pos.z ) continue;

	    if ( dobinid )
	    {
		const BinID bid = SI().transform( pos );
		strm << bid.inl() << od_tab << bid.crl();
	    }
	    else
	    {
		strm << pos.x << od_tab; // keep sep from next line
		strm << pos.y;
	    }

	    float z = (float) pos.z;
	    if ( infeet ) z *= mToFeetFactorF;
	    else if (intime )
	    {
		z = wd.d2TModel()->getTime( md, wd.track() );
		if ( inmsec && !mIsUdf(z) ) z *= cTWTFac;
	    }

	    strm << od_tab << z;
	}

	for ( int logidx=0; logidx<wd.logs().size(); logidx++ )
	{
	    const Well::Log& log = wd.logs().getLog( logidx );
	    if ( !logsel_.isPresent( log.name() ) ) continue;
	    const float val = log.getValue( md );
	    if ( mIsUdf(val) )
		strm << od_tab << "1e30";
	    else
		strm << od_tab << val;
	}
	strm << od_newline;
    }
}


//============================================================================

uiNewWellDlg::uiNewWellDlg( uiParent* p )
        : uiGetObjectName(p,uiGetObjectName::Setup("New Well",mkWellNms())
				.inptxt("New well name") )
{
    colsel_ = new uiColorInput( this, uiColorInput::Setup(getRandStdDrawColor())
				      .lbltxt("Color") );
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
	mErrRet( "Please enter a name" )

    if ( nms_->isPresent(newnm) )
	mErrRet( "Please specify a new name.\n"
		 "Wells can be removed in 'Manage wells'" )

    name_ = newnm;
    return true;
}


const Color& uiNewWellDlg::getWellColor()
{
    return colsel_->color();
}


uiWellLogUOMDlg::uiWellLogUOMDlg( uiParent* p, Well::Log& wl )
    : uiDialog(p,uiDialog::Setup("",mNoDlgTitle,mNoHelpID))
    , log_(wl)
{
    BufferString ttl( "Edit unit of measure :" );
    ttl += wl.name();
    setCaption( ttl.buf() );
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, "Unit of measure" );
    unfld_ = lcb->box();
    const ObjectSet<const UnitOfMeasure>& uns( UoMR().all() );
    unfld_->addItem( "-" );
    for ( int idx=0; idx<uns.size(); idx++ )
	unfld_->addItem( uns[idx]->name() );

    const char* curruom = log_.unitMeasLabel();
    const UnitOfMeasure* uom = UnitOfMeasure::getGuessed( curruom );
    unfld_->setCurrentItem( uom ? uom->name() : 0 );
}


bool uiWellLogUOMDlg::acceptOK( CallBacker* )
{
    BufferString uiunit = unfld_->text();
    BufferString curlogunit = log_.unitMeasLabel();

    if ( uiunit.isEqual( "-" ) )
	curlogunit.setEmpty();

    const UnitOfMeasure* newuom = UnitOfMeasure::getGuessed( uiunit );
    if ( newuom )
    {
	if ( *newuom->symbol() != '\0' )
	    curlogunit = newuom->symbol();
	else if ( *newuom->name() != '\0' )
	    curlogunit = newuom->name();
    }

    log_.setUnitMeasLabel( curlogunit );

    return true;
}
