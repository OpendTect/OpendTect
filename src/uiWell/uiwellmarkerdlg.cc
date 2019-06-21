/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2003
________________________________________________________________________

-*/


#include "uiwellmarkerdlg.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uifilesel.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uistratlvlsel.h"
#include "uistrattreewin.h"
#include "uistrings.h"
#include "uitable.h"
#include "uitblimpexpdatasel.h"
#include "uitoolbutton.h"
#include "uilabel.h"

#include "ioobjctxt.h"
#include "file.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "randcolor.h"
#include "stratlevel.h"
#include "survinfo.h"
#include "tabledef.h"
#include "od_ostream.h"
#include "oddirs.h"
#include "od_istream.h"
#include "welldata.h"
#include "wellmanager.h"
#include "wellimpasc.h"
#include "welltrack.h"
#include "od_helpids.h"

static const int cNrEmptyRows = 5;

static const char* sKeyName()		{ return sKey::Name(); }
static const char* sKeyMD()		{ return sKey::MD(); }
static const char* sKeyTVD()		{ return sKey::TVD(); }
static const char* sKeyTVDSS()		{ return sKey::TVDSS(); }
static const char* sKeyColor()		{ return sKey::Color(); }
static const char* sKeyRegMarker()	{ return "Regional marker"; }
static const int cNameCol  = 0;
static const int cDepthCol = 1;
static const int cTVDCol = 2;
static const int cTVDSSCol = 3;
static const int cColorCol = 4;
static const int cLevelCol = 5;


static void getColumnLabels( BufferStringSet& lbls, uiCheckBox* unfld,
				bool withlvls )
{
    const bool zinfeet = unfld ? unfld->isChecked() : SI().depthsInFeet();

    lbls.add( sKeyName() );
    BufferString curlbl;

    curlbl = sKeyMD();
    curlbl.addSpace().add( getDistUnitString(zinfeet,true) );
    lbls.add( curlbl );

    curlbl = sKeyTVD();
    curlbl.addSpace().add( getDistUnitString(zinfeet,true) );
    lbls.add( curlbl );

    curlbl = sKeyTVDSS();
    curlbl.addSpace().add( getDistUnitString(zinfeet,true) );
    lbls.add( curlbl );

    lbls.add( sKeyColor() );
    if ( withlvls )
	lbls.add( sKeyRegMarker() );
}


static uiTable* createMarkerTable( uiParent* p, int nrrows, bool editable )
{
    uiTable* ret = new uiTable( p, uiTable::Setup()
					    .rowdesc(uiStrings::sMarker())
					    .rowgrow(editable).defrowlbl("")
					    .selmode(uiTable::Multi),
			  "Well Marker Table" );
    uiStringSet lbls;
    lbls.add( uiStrings::sName() )
	.add( uiStrings::sMD().withSurvZUnit() )
	.add( uiStrings::sTVD().withSurvZUnit() )
	.add( uiStrings::sTVDSS().withSurvZUnit() )
	.add( uiStrings::sColor() );
    if ( editable )
	lbls.add( uiStrings::sRegionalMarker() );
    ret->setColumnLabels( lbls );
    ret->setColumnResizeMode( uiTable::ResizeToContents );
    ret->setColumnStretchable( cLevelCol, true );
    ret->setNrRows( nrrows );
    ret->setColumnReadOnly( cColorCol, true );
    ret->setPrefWidth( 650 );

    return ret;
}


static float uiMarkerDlgzFactor( uiCheckBox* cb=0 )
{
    const bool isinm = cb ? !cb->isChecked() : !SI().depthsInFeet();

    if ( SI().zIsTime() )
	return isinm ? 1 : mToFeetFactorF;

    return ((SI().zInFeet() && !isinm) ||
	    (SI().zInMeter() && isinm)) ? 1
					  : ( SI().zInFeet() && isinm )
					      ? mFromFeetFactorF
					      : mToFeetFactorF;
}


void uiMarkerDlg::exportMarkerSet( uiParent* p, const Well::MarkerSet& mset,
			const Well::Track& trck, uiCheckBox* cb )
{
    uiFileSelector::Setup fssu; fssu.setForWrite();
    uiFileSelector uifs( p, fssu );
    if ( !uifs.go() )
	return;

    od_ostream strm( uifs.fileName() );
    if ( !strm.isOK() )
    {
	BufferString msg( "Cannot open '", strm.fileName(), "' for write" );
	strm.addErrMsgTo( msg );
	return;
    }

    BufferStringSet colnms;
    getColumnLabels( colnms, cb, false );
    strm << colnms.get( cDepthCol ) << od_tab
	 << colnms.get( cTVDCol ) << od_tab
	 << colnms.get( cTVDSSCol ) << od_tab
	 << colnms.get( cNameCol ) << od_newline;

    const float kbelev = trck.getKbElev();
    const float zfac = uiMarkerDlgzFactor( cb );

    Well::MarkerSetIter miter( mset );
    while( miter.next() )
    {
	const Well::Marker& mrkr = miter.get();
	const float dah = mrkr.dah();
	const float tvdss = mCast(float,trck.getPos(dah).z_);
	const float tvd = tvdss + kbelev;
	strm << dah * zfac << od_tab
	     << tvd * zfac << od_tab
	     << tvdss * zfac << od_tab
	     << mrkr.name() << od_newline;
    }
}


uiMarkerDlg::uiMarkerDlg( uiParent* p, const Well::Track& wd )
	: uiDialog(p,uiDialog::Setup(tr("Edit Well Markers"),mNoDlgTitle,
				     mODHelpKey(mMarkerDlgHelpID)))
	, track_(wd)
	, oldmrkrs_(0)
	, table_(0)
	, unitfld_(0)
{
    uiString title( toUiString("%1: %2") );
    title.arg( uiStrings::sWell() ).arg( wd.name() );
    setTitleText( title );

    table_ = new uiTable( this, uiTable::Setup().rowdesc(uiStrings::sMarker())
						.rowgrow(true)
						.defrowlbl("")
						.selmode(uiTable::Multi),
			  "Well Marker Table" );
    BufferStringSet header;
    getColLabels( header );
    table_->setColumnLabels( header.getUiStringSet() );
    table_->setColumnResizeMode( uiTable::ResizeToContents );
    table_->setColumnStretchable( cLevelCol, true );
    table_->setNrRows( cNrEmptyRows );
    table_->setColumnReadOnly( cColorCol, true );
    table_->setSelectionBehavior( uiTable::SelectRows );
    table_->valueChanged.notify( mCB(this,uiMarkerDlg,markerChangedCB) );
    table_->rowInserted.notify( mCB(this,uiMarkerDlg,markerAddedCB) );
    table_->setPrefWidth( 650 );

    uiButton* updatebut = new uiPushButton( this, tr("Update display"),
				mCB(this,uiMarkerDlg,updateDisplayCB), true );
    updatebut->attach( leftAlignedBelow, table_ );

    uiButton* rfbut = uiButton::getStd( this, OD::Import,
					mCB(this,uiMarkerDlg,rdFile), false );
    rfbut->attach( rightOf, updatebut );

    uiButton* expbut = uiButton::getStd( this, OD::Export,
					mCB(this,uiMarkerDlg,exportCB), false );
    expbut->attach( rightOf, rfbut );

    uiPushButton* setregmrkar =
	new uiPushButton( this,	tr("Set as regional markers"),
			  mCB(this,uiMarkerDlg,setAsRegMarkersCB), false );
    setregmrkar->attach( alignedBelow, updatebut );

    uiToolButton* stratbut = new uiToolButton( this, "man_strat",
				tr("Edit Stratigraphy to define Levels"),
				mCB(this,uiMarkerDlg,doStrat) );
    stratbut->attach( rightOf, setregmrkar );

    uiToolButton* randclrbut = new uiToolButton( this, "random_color",
				tr("Assign random colors to markers"),
				mCB(this,uiMarkerDlg,assignRandomColorsCB) );
    randclrbut->attach( rightOf, stratbut );

    unitfld_ = new uiCheckBox( this, tr("Z in Feet") );
    unitfld_->attach( rightAlignedBelow, table_ );
    unitfld_->setChecked( SI().depthsInFeet() );
    unitfld_->activated.notify( mCB(this,uiMarkerDlg,unitChangedCB) );

    setPrefWidthInChar( 60 );
}


uiMarkerDlg::~uiMarkerDlg()
{
    delete oldmrkrs_;
}


void uiMarkerDlg::getColLabels( BufferStringSet& lbls ) const
{
    getColumnLabels( lbls, unitfld_, true );
}


int uiMarkerDlg::getNrRows() const
{
    for ( int idx=table_->nrRows()-1; idx>=0; idx-- )
    {
	const BufferString txt( table_->text( RowCol(idx,cNameCol) ) );
	if ( !txt.isEmpty() ) return idx+1;
    }

    return 0;
}


bool uiMarkerDlg::getFromScreen()
{
    Well::MarkerSet markers;
    if ( !getMarkerSet(markers) )
	return false;

    setMarkerSet( markers );
    return true;
}


void uiMarkerDlg::markerAddedCB( CallBacker* )
{
    uiStratLevelSel* levelsel = new uiStratLevelSel( 0, true,
						    uiString::empty() );
    levelsel->selChange.notify( mCB(this,uiMarkerDlg,stratLvlChg) );
    const int currentrow = table_->currentRow();
    const Color defgreycol( 128, 128, 128 );
    table_->setCellGroup( RowCol(currentrow,cLevelCol), levelsel );
    setColorCell( currentrow, defgreycol );
    levelsel->setSensitive( true );
}


void uiMarkerDlg::markerChangedCB( CallBacker* )
{
    const RowCol rc = table_->notifiedCell();
    const int row = rc.row();
    const int col = rc.col();

    const bool depthchg = col == cDepthCol || col == cTVDCol || col==cTVDSSCol;
    const bool nmchg = col == cNameCol;

    if ( depthchg )
    {
	if ( mIsUdf(table_->getFValue(rc)) )
	{
	    uiMSG().error( uiStrings::phrEnter(tr("a valid number")) );
	    return;
	}
	if ( !updateMarkerDepths(row,col==cDepthCol) )
	    return;
    }
    else if ( nmchg )
    {
	const BufferString markernm( table_->text( rc ) );
	if ( markernm.isEmpty() )
	{
	    uiMSG().error( uiStrings::phrEnterValidName() );
	    return;
	}
    }

    uiObject* obj = table_->getCellObject( RowCol(row,cLevelCol) );
    if ( obj )
	obj->setSensitive( true );
}


float uiMarkerDlg::zFactor() const
{
    return uiMarkerDlgzFactor( unitfld_ );
}


void uiMarkerDlg::unitChangedCB( CallBacker* )
{
    NotifyStopper notifystop( table_->valueChanged );
    BufferStringSet colnms;
    getColLabels( colnms );
    table_->setColumnLabels( colnms.getUiStringSet() );
    const float zfac = unitfld_->isChecked() ? mToFeetFactorF
					     : mFromFeetFactorF;

    TypeSet<int> depthcols;
    depthcols += cDepthCol;
    depthcols += cTVDCol;
    depthcols += cTVDSSCol;
    for ( int rowidx=0; rowidx<table_->nrRows(); rowidx++ )
    {
	for ( int idx=0; idx<depthcols.size(); idx++ )
	{
	    const RowCol rc( rowidx, depthcols[idx] );
	    const float val = table_->getFValue( rc );
	    if ( mIsUdf(val) )
		continue;

	    table_->setValue( rc, val * zfac );
	}
    }
}



void uiMarkerDlg::setColorCell( int rowidx, const Color& col )
{
    const RowCol colrc = RowCol( rowidx, cColorCol );
    const RowCol lvlrc = RowCol( rowidx, cLevelCol );
    uiGroup* grp = table_->getCellGroup( lvlrc );
    mDynamicCastGet( uiStratLevelSel*, levelsel, grp )
    const bool havelvlsel = levelsel && levelsel->getID().isValid();
    table_->setCellReadOnly( colrc, havelvlsel );
    if ( !havelvlsel )
	table_->setColorSelectionCell( colrc, false );
    table_->setCellColor( colrc, col );
}


void uiMarkerDlg::doStrat( CallBacker* )
{
    StratTWin().popUp();
}


int uiMarkerDlg::rowNrFor( uiStratLevelSel* lvlsel ) const
{
    const int nrrows = table_->nrRows();
    for ( int irow=0; irow<nrrows; irow++ )
    {
	if ( lvlsel == table_->getCellGroup( RowCol(irow,cLevelCol) ) )
	    return irow;
    }

    return -1;
}


void uiMarkerDlg::setMarkerSet( const Well::MarkerSet& markers, bool add )
{
    const int nrnew = markers.size();
    NotifyStopper notifystop( table_->valueChanged );
    int startrow = add ? getNrRows() : 0;
    const int nrrows = nrnew + startrow + cNrEmptyRows;
    if ( !add ) table_->clearTable();
    table_->setNrRows( nrrows );
    const float zfac = zFactor();
    const float kbelev = track_.getKbElev();
    for ( int idx=0; idx<nrnew; idx++ )
    {
	const int irow = startrow + idx;
	uiGroup* grp = table_->getCellGroup( RowCol(irow,cLevelCol) );
	mDynamicCastGet(uiStratLevelSel*,levelsel,grp);
	if ( !levelsel )
	{
	    levelsel = new uiStratLevelSel(0, true, uiString::empty());
	    levelsel->selChange.notify( mCB(this,uiMarkerDlg,stratLvlChg) );
	    table_->setCellGroup( RowCol(irow,cLevelCol), levelsel );
	}

	const Well::Marker marker = markers.validIdx(idx)? markers.getByIdx(idx)
							  : Well::Marker::udf();
	if ( marker.isUdf() )
	{
	    Well::Marker mrk("");
	    levelsel->setSensitive( true );
	    table_->setText( RowCol(irow,cDepthCol), "" );
	    table_->setText( RowCol(irow,cTVDCol), "" );
	    table_->setText( RowCol(irow,cTVDSSCol), "" );
	    table_->setText( RowCol(irow,cNameCol), "" );
	    setColorCell( irow, mrk.color() );
	}
	else
	{
	    if ( !Strat::LVLS().isPresent(marker.levelID()) )
	    {
		Well::Marker mrkr = markers.getByIdx( idx );
		mrkr.setLevelID( Strat::Level::ID::getInvalid() );
		const_cast<Well::MarkerSet&>( markers).set( mrkr );
	    }

	    levelsel->setID( marker.levelID() );
	    const float dah = marker.dah();
	    table_->setText( RowCol(irow,cNameCol), marker.name() );
	    table_->setValue( RowCol(irow,cDepthCol), dah*zfac, 2 );
	    const float tvdss = mCast(float,track_.getPos(dah).z_);
	    table_->setValue( RowCol(irow,cTVDCol), (tvdss+kbelev)*zfac, 2 );
	    table_->setValue( RowCol(irow,cTVDSSCol), tvdss*zfac, 2 );
	    setColorCell( irow, marker.color() );
	    if ( marker.levelID().isValid() )
		updateFromLevel( irow, levelsel );

	    continue;
	}
    }

    table_->resizeColumnsToContents();
    table_->setColumnStretchable( cLevelCol, true );

    if ( !oldmrkrs_ )
    {
	oldmrkrs_ = new Well::MarkerSet();
	getMarkerSet( *oldmrkrs_ );
    }
}


void uiMarkerDlg::stratLvlChg( CallBacker* cb )
{
    mDynamicCastGet(uiStratLevelSel*,levelsel,cb)
    if ( !levelsel ) return;

    const int irow = rowNrFor( levelsel );
    if ( irow < 0 ) return;

    updateFromLevel( irow, levelsel );
}


void uiMarkerDlg::updateFromLevel( int irow, uiStratLevelSel* levelsel )
{
    if ( !levelsel ) return;

    NotifyStopper notifystop( table_->valueChanged );
    const bool havelvl = levelsel->getID().isValid();
    if ( havelvl )
    {
	setColorCell( irow, levelsel->getColor() );
	table_->setText( RowCol(irow,cNameCol), levelsel->getLevelName() );
    }

    table_->setCellReadOnly( RowCol(irow,cNameCol), havelvl );
}


class uiReadMarkerFile : public uiDialog
{ mODTextTranslationClass(uiReadMarkerFile);
public:

uiReadMarkerFile( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Import Markers"),mNoDlgTitle,
				 mODHelpKey(mReadMarkerFileHelpID) ))
    , fd_(*Well::MarkerSetAscIO::getDesc())
{
    setOkText( uiStrings::sImport() );
    uiFileSel::Setup fssu; fssu.withexamine( true );
    fnmfld_ = new uiFileSel( this, uiStrings::sInputASCIIFile(), fssu );

    dataselfld_ = new uiTableImpDataSel( this, fd_,
		      mODHelpKey(mTableImpDataSelmarkersHelpID) );
    dataselfld_->attach( alignedBelow, fnmfld_ );

    replfld_ = new uiGenInput( this, tr("Existing markers (if any)"),
			BoolInpSpec(true,uiStrings::sReplace(),
					 uiStrings::sKeep()) );
    replfld_->attach( alignedBelow, dataselfld_ );

    colorfld_ = new uiCheckBox( this, tr("Assign random colors to markers") );
    colorfld_->setChecked( false );
    colorfld_->attach( alignedBelow, replfld_ );
}

~uiReadMarkerFile()
{
    delete &fd_;
}

bool acceptOK()
{
    fnm_ = fnmfld_->fileName();
    if ( File::isEmpty(fnm_) )
	{ uiMSG().error( uiStrings::sInvInpFile() ); return false; }

    if ( !dataselfld_->commit() )
	return false;

    keep_ = !replfld_->getBoolValue();
    assignrandcolors_ = colorfld_->isChecked();
    return true;
}

    Table::FormatDesc&	fd_;
    BufferString	fnm_;
    bool		keep_;
    bool		assignrandcolors_;

    uiFileSel*		fnmfld_;
    uiGenInput*		replfld_;
    uiTableImpDataSel*	dataselfld_;
    uiCheckBox*		colorfld_;
};


void uiMarkerDlg::assignRandomColorsCB( CallBacker* )
{
    Well::MarkerSet mrkrs;
    getMarkerSet( mrkrs );
    assignRandomColors( mrkrs );
    setMarkerSet( mrkrs, false );
}


void uiMarkerDlg::assignRandomColors( Well::MarkerSet& mrkrs )
{
    Well::MarkerSetIter4Edit mrkriter( mrkrs );
    while ( mrkriter.next() )
	mrkriter.get().setColor( getRandomColor() );
}


void uiMarkerDlg::rdFile( CallBacker* )
{
    uiReadMarkerFile dlg( this );
    if ( !dlg.go() ) return;

    od_istream strm( dlg.fnm_ );
    if ( !strm.isOK() )
	{ uiMSG().error( tr("Input file exists but cannot be read") ); return; }

    Well::MarkerSetAscIO aio( dlg.fd_ );
    Well::MarkerSet mrkrs;
    aio.get( strm, mrkrs, track_ );
    if ( mrkrs.isEmpty() )
    {
	uiMSG().error( tr("No valid markers found") );
	return;
    }

    if ( dlg.assignrandcolors_ )
	assignRandomColors( mrkrs );

    setMarkerSet( mrkrs, dlg.keep_ );
}


bool uiMarkerDlg::getMarkerSet( Well::MarkerSet& markers ) const
{
    markers.setEmpty();
    BufferStringSet markernms;
    uiString errmsg;
    const float zfac = zFactor();
    for ( int rowidx=0; rowidx<table_->nrRows(); rowidx++ )
    {
	float dah = table_->getFValue( RowCol(rowidx,cDepthCol) );
	if ( mIsUdf(dah) )
	    continue;

	const BufferString markernm( table_->text( RowCol(rowidx,cNameCol) ) );
	if ( markernm.isEmpty() )
	    continue;

	if ( !markernms.addIfNew(markernm) )
	{
	    errmsg = tr("Marker name '%1' is present several times,"
			" please make sure it is unique")
		   .arg(markernm);
	    uiMSG().error( errmsg );
	    return false;
	}

	dah /= zfac;
	Well::Marker marker( markernm, dah );
	marker.setColor( table_->getCellColor(RowCol(rowidx,cColorCol)) );
	uiGroup* grp = table_->getCellGroup( RowCol(rowidx,cLevelCol) );
	mDynamicCastGet(uiStratLevelSel*,levelsel,grp)
	marker.setLevelID( levelsel ? levelsel->getID()
				     : Strat::Level::ID::getInvalid() );
	markers.add( marker );
    }

    return true;
}


bool uiMarkerDlg::acceptOK()
{
    Well::MarkerSet markers;
    if ( !getMarkerSet(markers) ) return false;

    Interval<float> dahrg( track_.dahRange() );
    const float zfac = zFactor();
    dahrg.scale( zFactor() );
    uiString errmsg;
    Well::MarkerSetIter miter( markers );
    while( miter.next() )
    {
	const float val = miter.getDah() * zfac;
	if ( !dahrg.includes(val,true) )
	    errmsg = toUiString("'%1' %2").arg(miter.markerName());
    }

    if ( !errmsg.isEmpty() )
    {
	errmsg.arg(tr("depth value(s) is/are out of well track range [%1-%2]. "
		      "Press Abort if you want to re-enter the depth.")
		 .arg(dahrg.start).arg(dahrg.stop));
      const bool res = uiMSG().askContinue( errmsg );
      if ( !res )
	  return false;
    }

    return true;
}


class uiMarkersList : public uiDialog
{ mODTextTranslationClass(uiMarkersList);
public:

uiMarkersList( uiParent* p, const Well::MarkerSet& mset )
	: uiDialog( p,uiDialog::Setup( tr("Markers List"), tr("Select markers"),
					mNoHelpKey) )
{
    list_ = new uiListBox( this, "Markers" );
    list_->setMultiChoice( true );
    Well::MarkerSetIter miter( mset );
    while( miter.next() )
    {
	const Well::Marker mrkr = miter.get();
	list_->addItem( toUiString(mrkr.name()), mrkr.color() );
    }
}

void getSelIDs( TypeSet<int>& items )
{ list_->getChosen( items ); }

protected:
	uiListBox*	list_;
};


void uiMarkerDlg::setAsRegMarkersCB( CallBacker* )
{
    Well::MarkerSet mset;
    if ( !getMarkerSet(mset) ) return;

    if ( !mset.size() )
    {
	uiMSG().error( tr("No markers available") );
	return;
    }

    uiMarkersList dlg( this, mset );
    if ( !dlg.go() )
	return;

    TypeSet<int> selitems;
    dlg.getSelIDs( selitems );
    if ( !selitems.size() )
	{ uiMSG().error( tr("No markers selected") ); return; }

    MonitorLock ml( mset );
    Strat::LevelSet& lvls = Strat::eLVLS();
    BufferStringSet alreadysetnms;
    uiString msg;
    for ( int idx=0; idx<selitems.size(); idx++ )
    {
	const int selidx = selitems[idx];
	const BufferString mnm = mset.getByIdx(selidx).name().buf();
	if ( lvls.isPresent(mnm) )
	    alreadysetnms.add( mnm );
    }

    ml.unlockNow();

    if ( !alreadysetnms.isEmpty()
      && !uiMSG().askContinue( tr("%1 already set as regional marker")
			.arg(alreadysetnms.getDispString(3)) ) )
	    return;

    ml.reLock();
    for ( int idx=0; idx<selitems.size(); idx++ )
    {
	const int selidx = selitems[idx];
	Well::Marker mrkr = mset.getByIdx(selidx);
	Strat::Level level( mrkr.name(), mrkr.color() );
	Strat::Level::ID lvlid = lvls.set( level );
	lvls.store( Repos::Survey );
	mrkr.setLevelID( lvlid );
	mset.set( mrkr );
    }
    ml.unlockNow();

    setMarkerSet( mset, false );
}


void uiMarkerDlg::exportCB( CallBacker* )
{
    Well::MarkerSet mset;
    if ( !getMarkerSet( mset ) )
	return;

    if ( mset.isEmpty() )
    {
	uiMSG().error( tr("No data available to export") );
	return;
    }

    exportMarkerSet( this, mset, track_, unitfld_ );
}


bool uiMarkerDlg::getKey( DBKey& wellid ) const
{
    wellid = Well::MGR().getIDByName( track_.name() );
    return wellid.isValid();
}


void uiMarkerDlg::updateDisplayCB( CallBacker* )
{
    DBKey wellid;
    if ( !getKey(wellid) )
	return;

    uiRetVal uirv;
    RefMan<Well::Data> wd = Well::MGR().fetchForEdit( wellid, Well::LoadReqs(),
							uirv );
    if ( !wd )
	{ uiMSG().error( uirv ); return; }

    getMarkerSet( wd->markers() );
}


bool uiMarkerDlg::rejectOK()
{
    DBKey wellid;
    if ( !getKey(wellid) )
	return true;

    uiRetVal uirv;
    RefMan<Well::Data> wd = Well::MGR().fetchForEdit( wellid,
			Well::LoadReqs(Well::Mrkrs), uirv );
    if ( !wd )
	return true;
    if ( oldmrkrs_ )
	wd->markers() = *oldmrkrs_;
    return true;
}



bool uiMarkerDlg::updateMarkerDepths( int rowidx, bool md2tvdss )
{
    NotifyStopper ns( table_->valueChanged );
    const RowCol rcin = table_->notifiedCell();
    const int row = rcin.row();
    const bool istvd = rcin.col() == cTVDCol;
    const float kbelev = track_.getKbElev();
    const float zfac = zFactor();

    float inval = table_->getFValue( rcin ) / zfac;
    if ( istvd ) inval -= kbelev;

    Interval<float> trckrg( md2tvdss ? track_.dahRange() : track_.zRange() );
    if ( !trckrg.includes(inval,true) )
    {
	uiString errmsg = tr("The entered depth %1 is outside of "
			     "track range\n[%2-%3] %4 %5")
		    .arg( inval * zfac )
		    .arg( trckrg.start * zfac )
		    .arg( trckrg.stop * zfac )
		    .arg( uiStrings::sDistUnitString(!unitfld_->isChecked()) )
		    .arg( md2tvdss ? sKeyMD() : istvd ? sKeyTVD()
						      : sKeyTVDSS() );
	Well::Marker marker = getMarker( row, true );
	uiMSG().error( errmsg );
	if ( !marker.isUdf() )
	    table_->setValue( rcin, getOldMarkerVal(marker) );
	else
	    table_->setText(rcin, uiString::empty());

	return false;
    }

    const float dah = md2tvdss ? inval : track_.getDahForTVD( inval );
    const float tvdss = md2tvdss ? mCast(float,track_.getPos(inval).z_) : inval;
    const float tvd = tvdss + kbelev;
    if ( !md2tvdss )
	table_->setValue( RowCol(row,cDepthCol), dah * zfac );

    if ( md2tvdss || istvd )
	table_->setValue( RowCol(row,cTVDSSCol), tvdss * zfac );

    if ( md2tvdss || !istvd )
	table_->setValue( RowCol(row,cTVDCol), tvd * zfac );

    return true;
}


Well::Marker uiMarkerDlg::getMarker( int row, bool fromname ) const
{
    Well::MarkerSet markers;
    if ( !getMarkerSet(markers) )
	return 0;

    const BufferString markernm( table_->text(RowCol(row,cNameCol)) );
    const float dah = table_->getFValue( RowCol(row,cDepthCol) );
    if ( (fromname && markernm.isEmpty()) || (!fromname && mIsUdf(dah)) )
	return 0;

    const int markeridx = markers.getIdxAbove( dah / zFactor() );
    if ( ( fromname && markers.getByName(markernm).isUdf() ) ||
	 (!fromname && !markers.validIdx(markeridx) ) )
	return 0;

    Well::Marker marker("");
    marker = fromname ? markers.getByName( markernm )
		      : markers.getByIdx( markeridx );
    return marker;
}


float uiMarkerDlg::getOldMarkerVal( const Well::Marker& marker ) const
{
    if ( marker.isUdf() )
	return mUdf(float);

    const RowCol rc = table_->notifiedCell();
    const bool ismd = rc.col() == cDepthCol;
    const bool istvd = rc.col() == cTVDCol;
    const float kbelev = track_.getKbElev();

    const float olddah = marker.dah();
    const float oldtvdss = mCast(float,track_.getPos(olddah).z_);
    const float oldtvd = oldtvdss + kbelev;
    const float oldval = ismd ? olddah : istvd ? oldtvd : oldtvdss;

    return oldval * zFactor();
}



uiMarkerViewDlg::uiMarkerViewDlg( uiParent* p, const Well::Data& wd )
    : uiDialog(p,uiDialog::Setup(uiStrings::sWellMarker(mPlural),
				mNoDlgTitle, mODHelpKey(mMarkerViewDlgHelpID)))
    , table_(0)
    , wd_(&wd)
{
    if ( !wd_ )
	return;

    setCtrlStyle( CloseOnly );
    setTitleText( tr("Markers for well '%1'").arg(toUiString(wd_->name())) );

    const Well::MarkerSet& mset = wd_->markers();
    if ( mset.isEmpty() )
	{ new uiLabel( this, tr("No markers for this well") ); return; }

    const int nrmrks = mset.size();
    table_ = createMarkerTable( this, nrmrks, false );

    const float zfac = uiMarkerDlgzFactor();
    const Well::Track& trck = wd_->track();
    const float kbelev = trck.getKbElev();
    Well::MarkerSetIter miter( mset );
    while( miter.next() )
    {
	const Well::Marker& mrkr = miter.get();
	const int irow = miter.curIdx();
	table_->setText( RowCol(irow,cNameCol), mrkr.name() );
	table_->setColorSelectionCell( RowCol(irow,cColorCol), false );
	table_->setCellColor( RowCol(irow,cColorCol), mrkr.color() );

	const float dah = mrkr.dah();
	table_->setValue( RowCol(irow,cDepthCol), dah * zfac );
	const float tvdss = (float)trck.getPos(dah).z_;
	table_->setValue( RowCol(irow,cTVDCol), (tvdss+kbelev) * zfac );
	table_->setValue( RowCol(irow,cTVDSSCol), tvdss * zfac );
    }


    uiButton* expbut = new uiPushButton( this, uiStrings::sExport(),
				    mCB(this,uiMarkerViewDlg,exportCB), false );
    expbut->setIcon( "export" );
    expbut->attach( centeredBelow, table_ );
}


void uiMarkerViewDlg::exportCB( CallBacker* )
{
    if ( !wd_ )
	return;

    uiMarkerDlg::exportMarkerSet( this, wd_->markers(), wd_->track() );
}
