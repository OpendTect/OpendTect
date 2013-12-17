/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiwellmarkerdlg.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uistratlvlsel.h"
#include "uistrattreewin.h"
#include "uitable.h"
#include "uitblimpexpdatasel.h"
#include "uitoolbutton.h"

#include "ctxtioobj.h"
#include "file.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "od_ostream.h"
#include "oddirs.h"
#include "stratlevel.h"
#include "od_istream.h"
#include "survinfo.h"
#include "tabledef.h"
#include "welldata.h"
#include "wellman.h"
#include "wellimpasc.h"
#include "welltrack.h"
#include "welltransl.h"

static const int cNrEmptyRows = 5;

static const char* sKeyName()		{ return "Name"; }
static const char* sKeyMD()		{ return "MD"; }
static const char* sKeyTVD()		{ return "TVD"; }
static const char* sKeyTVDSS()		{ return "TVDSS"; }
static const char* sKeyColor()		{ return "Color"; }
static const char* sKeyRegMarker()	{ return "Regional marker"; }
static const int cNameCol  = 0;
static const int cDepthCol = 1;
static const int cTVDCol = 2;
static const int cTVDSSCol = 3;
static const int cColorCol = 4;
static const int cLevelCol = 5;


uiMarkerDlg::uiMarkerDlg( uiParent* p, const Well::Track& t )
	: uiDialog(p,uiDialog::Setup("Well Markers", "Edit markers", "107.1.1"))
    	, track_(t)
        , oldmrkrs_(0)
        , table_(0)
   	, unitfld_(0)
{
    table_ = new uiTable( this, uiTable::Setup().rowdesc("Marker")
	    				        .rowgrow(true)
					        .defrowlbl("")
						.selmode(uiTable::Multi),
			  "Well Marker Table" );
    BufferStringSet header;
    getColLabels( header );
    table_->setColumnLabels( header );
    table_->setColumnResizeMode( uiTable::ResizeToContents );
    table_->setColumnStretchable( cLevelCol, true );
    table_->setNrRows( cNrEmptyRows );
    table_->setColumnReadOnly( cColorCol, true );
    table_->doubleClicked.notify( mCB(this,uiMarkerDlg,mouseClick) );
    table_->valueChanged.notify( mCB(this,uiMarkerDlg,markerChangedCB) );
    table_->rowInserted.notify( mCB(this,uiMarkerDlg,markerAddedCB) );
    table_->setPrefWidth( 650 );

    uiButton* updatebut = new uiPushButton( this, "&Update display",
	    			mCB(this,uiMarkerDlg,updateDisplayCB), true );
    updatebut->attach( leftAlignedBelow, table_ );

    uiButton* rfbut = new uiPushButton( this, "&Read new",
	    				mCB(this,uiMarkerDlg,rdFile), false );
    rfbut->attach( rightOf, updatebut );

    uiButton* expbut = new uiPushButton( this, "&Export",
	    				mCB(this,uiMarkerDlg,exportCB), false );
    expbut->attach( rightOf, rfbut );

    uiPushButton* setregmrkar =
	new uiPushButton( this,	"Set as regional markers",
			  mCB(this,uiMarkerDlg,setAsRegMarkersCB), false );
    setregmrkar->attach( alignedBelow, updatebut );
    
    uiToolButton* stratbut = new uiToolButton( this, "man_strat",
	    			"Edit Stratigraphy to define Levels",
				mCB(this,uiMarkerDlg,doStrat) );
    stratbut->attach( rightOf, setregmrkar );

    unitfld_ = new uiCheckBox( this, "Z in Feet" );
    unitfld_->attach( rightAlignedBelow, table_ );
    unitfld_->setChecked( SI().depthsInFeet() );
    unitfld_->activated.notify( mCB(this,uiMarkerDlg,unitChangedCB) );

    setPrefWidthInChar( 60 );
}


uiMarkerDlg::~uiMarkerDlg()
{
    if ( oldmrkrs_ ) delete oldmrkrs_;
}


void uiMarkerDlg::getColLabels( BufferStringSet& lbls ) const
{
    bool zinfeet = false;
    if ( unitfld_ )
	zinfeet = unitfld_->isChecked();

    lbls.add( sKeyName() );
    BufferString curlbl;

    curlbl = sKeyMD();
    curlbl.add( "(" ).add( getDistUnitString(zinfeet,false) ).add( ")" );
    lbls.add( curlbl );

    curlbl = sKeyTVD();
    curlbl.add( "(" ).add( getDistUnitString(zinfeet,false) ).add( ")" );
    lbls.add( curlbl );

    curlbl = sKeyTVDSS();
    curlbl.add( "(" ).add( getDistUnitString(zinfeet,false) ).add( ")" );
    lbls.add( curlbl );

    lbls.add( sKeyColor() );
    lbls.add( sKeyRegMarker() );
}


int uiMarkerDlg::getNrRows() const
{
    for ( int idx=table_->nrRows()-1; idx>=0; idx-- )
    {
	const char* txt = table_->text( RowCol(idx,cNameCol) );
	if ( txt && *txt ) return idx+1;
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
    uiStratLevelSel* levelsel = new uiStratLevelSel( 0, true, 0 );
    levelsel->selChange.notify( mCB(this,uiMarkerDlg,stratLvlChg) );
    const int currentrow = table_->currentRow();
    const Color defgreycol( 128, 128, 128 );
    table_->setCellGroup( RowCol(currentrow,cLevelCol), levelsel );
    table_->setColor( RowCol(currentrow,cColorCol), defgreycol );
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
	if ( mIsUdf(table_->getfValue(rc)) )
	{
	    uiMSG().error( "Please enter a valid number" );
	    table_->clearCell( rc );
	    return;
	}
	if ( !updateMarkerDepths(row,col==cDepthCol) )
	    return;
    }
    else if ( nmchg )
    {
	const char* markernm = table_->text( rc );
	if ( !markernm || !*markernm )
	{
	    uiMSG().error( "Please enter a valid name" );
	    table_->clearCell( rc );
	    return;
	}
    }

    uiObject* obj = table_->getCellObject( RowCol(row,cLevelCol) );
    if ( obj )
	obj->setSensitive( true );
}


float uiMarkerDlg::zFactor() const
{
    const bool unitval = !unitfld_->isChecked();
    
    if ( SI().zIsTime() )
	return unitval ? 1 : mToFeetFactorF;

    return ((SI().zInFeet() && !unitval) ||
	    (SI().zInMeter() && unitval)) ? 1
					  : ( SI().zInFeet() && unitval )
					      ? mFromFeetFactorF
					      : mToFeetFactorF;
}


void uiMarkerDlg::unitChangedCB( CallBacker* )
{
    NotifyStopper notifystop( table_->valueChanged );
    BufferStringSet header;
    getColLabels( header );
    table_->setColumnLabels( header );
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
	    const float val = table_->getfValue( rc );
	    if ( mIsUdf(val) )
		continue;

	    table_->setValue( rc, val * zfac );
	}
    }
}


void uiMarkerDlg::mouseClick( CallBacker* )
{
    const RowCol rc = table_->notifiedCell();
    if ( rc.col() != cColorCol ) return;

    uiGroup* grp = table_->getCellGroup( RowCol(rc.row(),cLevelCol) );
    mDynamicCastGet(uiStratLevelSel*,levelsel,grp)
    const bool havelvl = levelsel && levelsel->getID() >= 0;
    if ( havelvl )
    {
	uiMSG().error( "Cannot change color of regional marker" );
	return;
    }

    Color newcol = table_->getColor( rc );
    if ( selectColor(newcol, this, "Marker color") )
	table_->setColor( rc, newcol );

    table_->setSelected( rc, false );
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
    table_->setNrRows( nrrows );
    const float zfac = zFactor();
    const float kbelev = track_.getKbElev();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	const int irow = startrow + idx;
	uiStratLevelSel* levelsel = new uiStratLevelSel( 0, true, 0 );
	levelsel->selChange.notify( mCB(this,uiMarkerDlg,stratLvlChg) );
	table_->setCellGroup( RowCol(irow,cLevelCol), levelsel );
	const Well::Marker* marker = markers.validIdx(idx) ? markers[idx] : 0;
	if ( marker )
	{
	    if ( !Strat::LVLS().isPresent( marker->levelID() ) )
		const_cast<Well::Marker*>(markers[idx])->setLevelID( -1 );

	    levelsel->setID( marker->levelID() );
	    const float dah = marker->dah();
	    table_->setValue( RowCol(irow,cDepthCol), dah * zfac );
	    const float tvdss = mCast(float,track_.getPos(dah).z);
	    table_->setValue( RowCol(irow,cTVDCol), (tvdss+kbelev) * zfac );
	    table_->setValue( RowCol(irow,cTVDSSCol), tvdss * zfac );
	    table_->setText( RowCol(irow,cNameCol), marker->name() );
	    table_->setColor( RowCol(irow,cColorCol), marker->color() );
	    if ( marker->levelID() >= 0 )
		updateFromLevel( irow, levelsel );

	    continue;
	}

	Well::Marker mrk;
	levelsel->setSensitive( true );
	table_->setText( RowCol(irow,cDepthCol), "" );
	table_->setText( RowCol(irow,cTVDCol), "" );
	table_->setText( RowCol(irow,cTVDSSCol), "" );
	table_->setText( RowCol(irow,cNameCol), "" );
	table_->setColor( RowCol(irow,cColorCol), mrk.color() );
    }

    table_->resizeHeaderToContents( false );

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
    const bool havelvl = levelsel->getID() >= 0;
    if ( havelvl )
    {
	table_->setColor( RowCol(irow,cColorCol), levelsel->getColor() );
	table_->setText( RowCol(irow,cNameCol), levelsel->getName() );
    }

    table_->setCellReadOnly( RowCol(irow,cNameCol), havelvl );
}


class uiReadMarkerFile : public uiDialog
{
public:

uiReadMarkerFile( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import Markers",
				 "Specify Marker import",
				 "107.1.4"))
    , fd_(*Well::MarkerSetAscIO::getDesc())
{
    fnmfld_ = new uiFileInput( this, "Input Ascii file",
	    		uiFileInput::Setup().withexamine(true)
					    .forread(true));

    dataselfld_ = new uiTableImpDataSel( this, fd_, "107.1.9" );
    dataselfld_->attach( alignedBelow, fnmfld_ );

    replfld_ = new uiGenInput( this, "Existing markers (if any)",
	    		      BoolInpSpec(true,"Replace","Keep") );
    replfld_->attach( alignedBelow, dataselfld_ );
}

~uiReadMarkerFile()
{
    delete &fd_;
}

bool acceptOK( CallBacker* )
{
    fnm_ = fnmfld_->fileName();
    if ( File::isEmpty(fnm_) )
	{ uiMSG().error( "Invalid input file" ); return false; }

    if ( !dataselfld_->commit() )
	return false;

    keep_ = !replfld_->getBoolValue();
    return true;
}

    Table::FormatDesc&	fd_;
    BufferString	fnm_;
    bool		keep_;

    uiFileInput*	fnmfld_;
    uiGenInput*		replfld_;
    uiTableImpDataSel*	dataselfld_;
};


void uiMarkerDlg::rdFile( CallBacker* )
{
    uiReadMarkerFile dlg( this );
    if ( !dlg.go() ) return;

    od_istream strm( dlg.fnm_ );
    if ( !strm.isOK() )
	{ uiMSG().error( "Input file exists but cannot be read" ); return; }

    Well::MarkerSetAscIO aio( dlg.fd_ );
    Well::MarkerSet mrkrs;
    aio.get( strm, mrkrs, track_ );
    if ( mrkrs.isEmpty() )
	uiMSG().error( "No valid markers found" );
    else
	setMarkerSet( mrkrs, dlg.keep_ );
}


bool uiMarkerDlg::getMarkerSet( Well::MarkerSet& markers ) const
{
    deepErase( markers );
    BufferStringSet markernms;
    BufferString errmsg;
    const float zfac = zFactor();
    for ( int rowidx=0; rowidx<table_->nrRows(); rowidx++ )
    {
	float dah = table_->getfValue( RowCol(rowidx,cDepthCol) );
	if ( mIsUdf(dah) )
	    continue;

	const char* markernm = table_->text( RowCol(rowidx,cNameCol) );
	if ( !markernm || !*markernm )
	    continue;

	if ( !markernms.addIfNew(markernm) )
	{
	    errmsg = "Marker name '";
	    errmsg.add( markernm ).add( "' is present several times," )
	          .add( " please make sure it is unique" );
	    uiMSG().error( errmsg );
	    return false;
	}

	dah /= zfac;
	Well::Marker* marker = new Well::Marker( markernm, dah );
	marker->setColor( table_->getColor(RowCol(rowidx,cColorCol)) );
	uiGroup* grp = table_->getCellGroup( RowCol(rowidx,cLevelCol) );
	mDynamicCastGet(uiStratLevelSel*,levelsel,grp)
	marker->setLevelID( levelsel ? levelsel->getID() : -1 );
	markers += marker;
    }

    return true;
}


bool uiMarkerDlg::acceptOK( CallBacker* )
{
    Well::MarkerSet markers;
    if ( !getMarkerSet(markers) ) return false;

    Interval<float> dahrg( track_.dahRange() );
    const float zfac = zFactor();
    dahrg.scale( zFactor() );
    BufferString errmsg;
    for ( int midx=0; midx<markers.size(); midx++ )
    {
	const float val = markers[midx]->dah() * zfac;
	if ( !dahrg.includes(val,true) )
	    errmsg.add( "'" ).add( markers[midx]->name() ).add( "' " );
    }

    if ( !errmsg.isEmpty() )
    {
      errmsg.add( "depth value(s) is/are out of well track range [" )
	    .add( dahrg.start ).add( "-" ).add( dahrg.stop ).add( "]. " )
	    .add ( "Press Abort if you want to re-enter the depth." );
      const bool res = uiMSG().askContinue( errmsg );
      if ( !res ) return false;
    }

    return true;
}


class uiMarkersList : public uiDialog
{
public:

uiMarkersList( uiParent* p, const Well::MarkerSet& mset )
	: uiDialog( p,uiDialog::Setup( "Markers List", "Select markers", 
		    			mNoHelpID) )
{
    list_ = new uiListBox( this, "Markers" );
    list_->setItemsCheckable( true );
    for ( int idx=0; idx<mset.size(); idx++ )
	list_->addItem( mset[idx]->name(), mset[idx]->color() );
}

void getSelIDs( TypeSet<int>& items )
{ list_->getCheckedItems( items ); }

protected:
	uiListBox*	list_;
};


bool uiMarkerDlg::setAsRegMarkersCB( CallBacker* )
{
    Well::MarkerSet mset;
    if ( !getMarkerSet( mset ) ) return false;

    if ( !mset.size() )
    {
	uiMSG().message( "No markers available" );
	return false;
    }

    uiMarkersList dlg( this, mset );
    if ( !dlg.go() ) return false;

    TypeSet<int> selitems;
    dlg.getSelIDs( selitems );
    if ( !selitems.size() )
    {
	uiMSG().message( "No markers selected." );
	return false;
    }

    Strat::LevelSet& lvls = Strat::eLVLS();
    BufferString msg;
    int mid = 0;
    for ( int idx=0; idx<selitems.size(); idx++ )
    {
	const int selidx = selitems[idx];
	if ( lvls.isPresent(mset[selidx]->name()) )
	{
	    msg.add( "'" ).add( mset[selidx]->name() ).add( "' ");
	    mid++;
	}
    }

    if ( !msg.isEmpty() )
    {
	msg.add( mid > 1 ? "are " : "is " );
	msg.add( "already set as regional marker(s)." );
	msg.add( "Press Continue to update properties." );
	const bool res = uiMSG().askContinue( msg );
	if ( !res ) return false;
    }

    for ( int idx=0; idx<selitems.size(); idx++ )
    {
	const int selidx = selitems[idx];
	Strat::Level* level = lvls.add( mset[selidx]->name(),
					mset[selidx]->color() );
	lvls.store( Repos::Survey );
	mset[selidx]->setLevelID( level->id() );
    }

    setMarkerSet( mset, false );
    return true;
}


void uiMarkerDlg::exportCB( CallBacker* )
{
    Well::MarkerSet mset;
    if ( !getMarkerSet( mset ) )
	return;

    if ( mset.isEmpty() )
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
	BufferString msg( "Cannot open '", fdlg.fileName(), "' for write" );
	strm.addErrMsgTo( msg );
	return;
    }

    BufferStringSet header;
    getColLabels( header );
    strm << header.get( cDepthCol ) << od_tab
	 << header.get( cTVDCol ) << od_tab
	 << header.get( cTVDSSCol ) << od_tab
	 << header.get( cNameCol ) << od_newline;
    
    const float kbelev = track_.getKbElev();
    const float zfac = zFactor();
    for ( int idx=0; idx<mset.size(); idx++ )
    {
	const float dah = mset[idx]->dah();
	const float tvdss = mCast(float,track_.getPos(dah).z);
	const float tvd = tvdss + kbelev;
	strm << dah * zfac << od_tab
	     << tvd * zfac << od_tab
	     << tvdss * zfac << od_tab
	     << mset[idx]->name() << od_newline;
    }
}


bool uiMarkerDlg::getKey( MultiID& mid ) const
{
    IOM().to( WellTranslatorGroup::ioContext().getSelKey() );
    IOObj* obj = IOM().getLocal( track_.name(), "Well" );
    if ( !obj )
	return false;

    mid = obj->key();
    return true;
}


void uiMarkerDlg::updateDisplayCB( CallBacker* )
{
    MultiID mid;
    if ( !getKey(mid) )
	return;

    Well::Data* wd =0;
    if ( Well::MGR().isLoaded( mid ) )
	wd = Well::MGR().get( mid );

    if ( !wd )
	return;

    getMarkerSet( wd->markers() );
    wd->markerschanged.trigger();
}


bool uiMarkerDlg::rejectOK( CallBacker* )
{
    MultiID mid;
    if ( !getKey(mid) )
	return true;

    Well::Data* wd =0;
    if ( Well::MGR().isLoaded( mid ) )
    {
	wd = Well::MGR().get( mid );
	if ( oldmrkrs_ && wd )
	{
	    deepCopy<Well::Marker,Well::Marker>( wd->markers(),*oldmrkrs_ );
	    wd->markerschanged.trigger();
	}
    }

    return true;
}


#define mDelRet { if (marker) delete marker; return false; }


bool uiMarkerDlg::updateMarkerDepths( int rowidx, bool md2tvdss )
{
    NotifyStopper ns( table_->valueChanged );
    const RowCol rcin = table_->notifiedCell();
    const int row = rcin.row();
    const bool istvd = rcin.col() == cTVDCol;
    const float kbelev = track_.getKbElev();
    const float zfac = zFactor();

    float inval = table_->getfValue( rcin ) / zfac;
    if ( istvd ) inval -= kbelev;

    Interval<float> trckrg( md2tvdss ? track_.dahRange() : track_.zRange() );
    if ( !trckrg.includes(inval,true) )
    {
	BufferString errmsg( "The entered depth " );
	errmsg.add( inval * zfac ).add( " is outside of track range\n" );
	errmsg.add( "[" ).add( trckrg.start * zfac ).add( ", ");
	errmsg.add( trckrg.stop * zfac ).add( "] " );
	errmsg.add( !unitfld_->isChecked() ? "m" : "ft" );
	errmsg.add( md2tvdss ? sKeyMD() : istvd ? sKeyTVD() : sKeyTVDSS() );
	Well::Marker* marker = getMarker( row, true );
	uiMSG().error( errmsg );
	if ( marker )
	    table_->setValue( rcin, getOldMarkerVal(marker) );
	else
	    table_->clearCell( rcin );

	mDelRet;
    }

    const float dah = md2tvdss ? inval : track_.getDahForTVD( inval );
    const float tvdss = md2tvdss ? mCast(float,track_.getPos(inval).z) : inval;
    const float tvd = tvdss + kbelev;
    if ( !md2tvdss )
	table_->setValue( RowCol(row,cDepthCol), dah * zfac );

    if ( md2tvdss || istvd )
	table_->setValue( RowCol(row,cTVDSSCol), tvdss * zfac );

    if ( md2tvdss || !istvd )
	table_->setValue( RowCol(row,cTVDCol), tvd * zfac );

    return true;
}


Well::Marker* uiMarkerDlg::getMarker( int row, bool fromname ) const
{
    Well::MarkerSet markers;
    if ( !getMarkerSet(markers) )
	return 0;

    const char* markernm = table_->text(RowCol(row,cNameCol));
    const float dah = table_->getfValue( RowCol(row,cDepthCol) );
    if ( (fromname && (!markernm || !*markernm)) ||
	 (!fromname && mIsUdf(dah)) )
	return 0;

    const int markeridx = markers.getIdxAbove( dah / zFactor() );
    if ( ( fromname && !markers.getByName(markernm) ) ||
	 (!fromname && !markers.validIdx(markeridx) ) )
	return 0;

    Well::Marker* marker = new Well::Marker();
    *marker = fromname ? *(markers.getByName( markernm ))
		       : *(markers[markeridx]);

    return marker;
}


float uiMarkerDlg::getOldMarkerVal( Well::Marker* marker ) const
{
    if ( !marker )
	return mUdf(float);

    const RowCol rc = table_->notifiedCell();
    const bool ismd = rc.col() == cDepthCol;
    const bool istvd = rc.col() == cTVDCol;
    const float kbelev = track_.getKbElev();

    const float olddah = marker->dah();
    const float oldtvdss = mCast(float,track_.getPos(olddah).z);
    const float oldtvd = oldtvdss + kbelev;
    const float oldval = ismd ? olddah : istvd ? oldtvd : oldtvdss;

    return oldval * zFactor();
}

