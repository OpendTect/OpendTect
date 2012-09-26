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
#include "oddirs.h"
#include "stratlevel.h"
#include "strmprov.h"
#include "survinfo.h"
#include "tabledef.h"
#include "welldata.h"
#include "wellman.h"
#include "wellimpasc.h"
#include "welltrack.h"
#include "welltransl.h"

static const char* mrkrcollbls[] = { "[Name]", "Depth (MD)", 
				 "[Color]", "Regional marker", 0 };
static const int cNrEmptyRows = 5;

static const int cNameCol  = 0;
static const int cDepthCol = 1;
static const int cColorCol = 2;
static const int cLevelCol = 3;


uiMarkerDlg::uiMarkerDlg( uiParent* p, const Well::Track& t )
	: uiDialog(p,uiDialog::Setup("Well Markers", "Edit markers", "107.1.1"))
    	, track_(t)
        , oldmrkrs_(0)
{
    table_ = new uiTable( this, uiTable::Setup().rowdesc("Marker")
	    				        .rowgrow(true) 
					        .defrowlbl("")
				,"Well Marker Table" );
    table_->setColumnLabels( mrkrcollbls );
    table_->setColumnResizeMode( uiTable::ResizeToContents );
    table_->setColumnStretchable( cLevelCol, true );
    table_->setNrRows( cNrEmptyRows );
    table_->setColumnReadOnly( cColorCol, true );
    table_->doubleClicked.notify( mCB(this,uiMarkerDlg,mouseClick) );
    table_->valueChanged.notify( mCB(this,uiMarkerDlg,markerChangedCB) );
    table_->rowInserted.notify( mCB(this,uiMarkerDlg,markerAddedCB) );
    table_->rowDeleted.notify( mCB(this,uiMarkerDlg,markerRemovedCB) );
    table_->setPrefWidth( 400 );

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
    unitfld_->activated.notify( mCB(this,uiMarkerDlg,unitChangedCB) );
    unitfld_->attach( rightAlignedBelow, table_ );
    unitfld_->setChecked( SI().depthsInFeetByDefault() );

    setPrefWidthInChar( 60 );
}


uiMarkerDlg::~uiMarkerDlg()
{
    if ( oldmrkrs_ ) delete oldmrkrs_;
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


void uiMarkerDlg::markerRemovedCB( CallBacker* )
{
    const int currow = table_->currentRow();
    depths_.remove( currow<0 ? table_->nrRows() : currow );
}


void uiMarkerDlg::markerAddedCB( CallBacker* )
{
    uiStratLevelSel* levelsel = new uiStratLevelSel( 0, true, 0 );
    levelsel->selChange.notify( mCB(this,uiMarkerDlg,stratLvlChg) );
    table_->setCellGroup( RowCol(table_->currentRow(),cLevelCol), levelsel );
    levelsel->setSensitive( false );
    depths_.insert( table_->currentRow(), 1e30 );
    markerChangedCB( 0 );
}


void uiMarkerDlg::markerChangedCB( CallBacker* )
{
    if ( table_->nrRows() != depths_.size() )
	return;

    const int row = table_->currentRow();
    const float val = table_->getfValue( RowCol(row,cDepthCol) );
    const char* markernm = table_->text(RowCol(row,cNameCol));
    if ( !markernm || !*markernm || mIsUdf(val) )
    {
	depths_[row] = 1e30;
	return;
    }

    depths_[row] = val / zFactor();
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
    if ( table_->nrRows() != depths_.size() )
	return;

    NotifyStopper notifystop( table_->valueChanged );
    table_->selectColumn( cNameCol );
    for ( int rowidx=0; rowidx<table_->nrRows(); rowidx++ )
    {
	const RowCol rc( rowidx, cDepthCol );
	const float val = depths_[rowidx];
	if ( mIsUdf(val) )
	{
	    table_->clearCell( rc );
	    continue;
	}

	table_->setValue( rc, val*zFactor() );
    }
    
    table_->selectColumn( cDepthCol );
}


void uiMarkerDlg::mouseClick( CallBacker* )
{
    const RowCol rc = table_->notifiedCell();
    if ( rc.col != cColorCol ) return;

    uiGroup* grp = table_->getCellGroup( RowCol(rc.row,cLevelCol) );
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
	    table_->setValue( RowCol(irow,cDepthCol), marker->dah()*zFactor() );
	    table_->setText( RowCol(irow,cNameCol), marker->name() );
	    table_->setColor( RowCol(irow,cColorCol), marker->color() );
	    if ( marker->levelID() >= 0 )
		updateFromLevel( irow, levelsel );

	    continue;
	}

	Well::Marker mrk;
	levelsel->setSensitive( false );
	table_->setText( RowCol(irow,cDepthCol), "" );
	table_->setText( RowCol(irow,cNameCol), "" );
	table_->setColor( RowCol(irow,cColorCol), mrk.color() );
    }

    depths_.erase();
    for ( int idx=0; idx<table_->nrRows(); idx++ )
    {
	const float val = table_->getfValue( RowCol(idx,cDepthCol) );
	if ( mIsUdf(val) )
	    depths_ += 1e30;
	else
	    depths_ += val/zFactor();
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

    const bool havelvl = levelsel->getID() >= 0;
    RowCol rc( irow, cColorCol );
    if ( havelvl )
    {
	rc.col = cColorCol; table_->setColor( rc, levelsel->getColor() );
	rc.col = cNameCol; table_->setText( rc, levelsel->getName() );
    }
    //TODO this will replace by former marker name
    //     but this does not handle add/remove marker.

    rc.col = cNameCol; table_->setCellReadOnly( rc, havelvl );
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

    StreamData sd( StreamProvider(dlg.fnm_).makeIStream() );
    if ( !sd.usable() )
	{ uiMSG().error( "Input file exists but cannot be read" ); return; }

    Well::MarkerSetAscIO aio( dlg.fd_ );
    Well::MarkerSet mrkrs;
    aio.get( *sd.istrm, mrkrs, track_ );
    if ( mrkrs.isEmpty() )
	uiMSG().error( "No valid markers found" );
    else
	setMarkerSet( mrkrs, dlg.keep_ );
}


bool uiMarkerDlg::getMarkerSet( Well::MarkerSet& markers )
{
    if ( table_->nrRows() != depths_.size() )
	return false;

    deepErase( markers );
    BufferStringSet markernms;
    BufferString errmsg;
    for ( int rowidx=0; rowidx<table_->nrRows(); rowidx++ )
    {
	const float depth = depths_[rowidx];
	const char* markernm = table_->text( RowCol(rowidx,cNameCol) );
	if ( !markernm || !*markernm || mIsUdf(depth) ) continue;

	if ( !markernms.addIfNew(markernm) )
	{
	    errmsg = "Marker name '";
	    errmsg.add( markernm ).add( "' is present several times," )
	          .add( " please make sure it is unique" );
	    uiMSG().error( errmsg );
	    return false;
	}

	Well::Marker* marker = new Well::Marker( markernm, depth );
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
    if ( !getMarkerSet( markers ) ) return false;

    Interval<float> dahrg = track_.dahRange();
    dahrg.start = dahrg.start * zFactor();
    dahrg.stop = dahrg.stop * zFactor();
    BufferString errmsg;
    for ( int midx=0; midx<markers.size(); midx++ )
    {
	const float val = markers[midx]->dah() * zFactor();
	const bool isbetween = dahrg.includes( val, true );
	const RowCol rcname( midx, cNameCol );
	if ( !isbetween )
	    errmsg.add( "'" ).add( markers[midx]->name() ).add( "' " );
    }

    if ( !errmsg.isEmpty() )
    {
      errmsg.add( "depth value(s) is out of well track range [" )
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

    StreamData sd( StreamProvider(fdlg.fileName()).makeOStream() );
    if ( !sd.usable() )
    {
	uiMSG().error( BufferString( "Cannot open '", fdlg.fileName(),
		    		     "' for write" ) );
	return;
    }
    
    for ( int idx=0; idx<mset.size(); idx++ )
    {
	*sd.ostrm << mset[idx]->dah()*zFactor() << '\t';
   	*sd.ostrm << mset[idx]->name() << '\n';
    }

    sd.close();
}


bool uiMarkerDlg::getKey( MultiID& mid ) const
{
    IOM().to( WellTranslatorGroup::ioContext().getSelKey() );
    IOObj* obj = IOM().getLocal( track_.name() );
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
