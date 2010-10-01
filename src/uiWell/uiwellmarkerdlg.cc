/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellmarkerdlg.cc,v 1.31 2010-10-01 16:13:57 cvsbruno Exp $";


#include "uiwellmarkerdlg.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uistratlvlsel.h"
#include "uistrattreewin.h"
#include "uitblimpexpdatasel.h"
#include "uitable.h"

#include "ctxtioobj.h"
#include "file.h"
#include "iopar.h"
#include "pixmap.h"
#include "tabledef.h"
#include "strmprov.h"
#include "survinfo.h"
#include "stratlevel.h"
#include "welldata.h"
#include "wellimpasc.h"
#include "wellmarker.h"
#include "welltrack.h"



static const char* mrkrcollbls[] = { "[Name]", "Depth (MD)", 
				 "[Color]", "Stratigraphic marker", 0 };
static const int cNrEmptyRows = 5;

static const int cNameCol  = 0;
static const int cDepthCol = 1;
static const int cColorCol = 2;
static const int cLevelCol = 3;


uiMarkerDlg::uiMarkerDlg( uiParent* p, const Well::Track& t )
	: uiDialog(p,uiDialog::Setup("Well Markers",
				     "Define marker properties",
				     "107.1.1"))
    	, track_(t)
{
    table_ = new uiTable( this, uiTable::Setup().rowdesc("Marker")
	    				        .rowgrow(true) 
					        .defrowlbl("")
				,"Well Marker Table" );
    table_->setColumnLabels( mrkrcollbls );
    table_->setColumnReadOnly( cColorCol, true );
    table_->setColumnResizeMode( uiTable::ResizeToContents );
    table_->setColumnStretchable( cLevelCol, true );
    table_->setNrRows( cNrEmptyRows );
    table_->leftClicked.notify( mCB(this,uiMarkerDlg,mouseClick) );

    BoolInpSpec mft( !SI().depthsInFeetByDefault(), "Meter", "Feet" );
    unitfld_ = new uiGenInput( this, "Depth unit", mft );
    unitfld_->attach( leftAlignedBelow, table_ );

    uiButton* rfbut = new uiPushButton( this, "&Read file",
	    				mCB(this,uiMarkerDlg,rdFile), false );
    rfbut->attach( rightTo, unitfld_ ); rfbut->attach( rightBorder );
    
    uiToolButton* sb = new uiToolButton( this, "Create new Levels",
					ioPixmap("man_strat.png"),
					mCB(this,uiMarkerDlg,doStrat) );
    sb->setToolTip( "Edit Stratigraphy to define Levels" );
    sb->attach( leftOf, rfbut );

    stratmrkfld_ = new uiCheckBox( this, "Set as regional markers" );
    stratmrkfld_->attach( ensureBelow, unitfld_ ); 
    stratmrkfld_->attach( hCentered ); 
    stratmrkfld_->setChecked( false ); 

    setPrefWidthInChar( 60 );
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


void uiMarkerDlg::mouseClick( CallBacker* )
{
    RowCol rc = table_->notifiedCell();
    if ( rc.col != cColorCol || table_->isCellReadOnly(rc) ) return;

    Color newcol = table_->getColor( rc );
    if ( selectColor(newcol,this,"Marker color") )
	table_->setColor( rc, newcol );
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
    if ( !nrnew ) return;

    const float zfac = unitfld_->getBoolValue() ? 1 : mToFeetFactor;
    int startrow = add ? getNrRows() : 0;
    const int nrrows = nrnew + cNrEmptyRows + startrow;
    table_->setNrRows( nrrows );

    markers_.erase();
    for ( int idx=0; idx<nrnew; idx++ )
    {
	int irow = startrow + idx;
	const Well::Marker* marker = markers[idx];
	markers_ += marker;
	if ( !Strat::LVLS().isPresent( marker->levelID() ) )
	    const_cast<Well::Marker*>(markers[idx])->setLevelID( -1 );
	uiStratLevelSel* levelsel = new uiStratLevelSel( 0, false );
	levelsel->selChange.notify( mCB(this,uiMarkerDlg,stratLvlChg) );
	table_->setCellGroup( RowCol(irow,cLevelCol), levelsel );
	levelsel->setID( marker->levelID() );
	table_->setValue( RowCol(irow,cDepthCol), marker->dah()*zfac );
	table_->setText( RowCol(irow,cNameCol), marker->name() );
	table_->setColor( RowCol(irow,cColorCol), marker->color() );
	if ( marker->levelID() >= 0 )
	    updateFromLevel( irow, levelsel );
    }
    Well::Marker mrk;
    for ( int irow=startrow+nrnew; irow<nrrows; irow++ )
    {
	uiStratLevelSel* levelsel = new uiStratLevelSel( 0, false );
	levelsel->selChange.notify( mCB(this,uiMarkerDlg,stratLvlChg) );
	levelsel->setSensitive( false );
	table_->setCellGroup( RowCol(irow,cLevelCol), levelsel );
	table_->setText( RowCol(irow,cDepthCol), "" );
	table_->setText( RowCol(irow,cNameCol), "" );
	table_->setColor( RowCol(irow,cColorCol), mrk.color() );
	updateFromLevel( irow, levelsel );
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
    const bool havelvl = ( levelsel->getID() >= 0 );
    RowCol rc( irow, cColorCol );
    if ( havelvl )
    {
	rc.col = cColorCol; table_->setColor( rc, levelsel->getColor() );
	rc.col = cNameCol; table_->setText( rc, levelsel->getName() );
    }
    //TODO this will replace by former marker name
    //     but this does not handle add/remove marker.
    else if ( irow < markers_.size() )
    {
	const Well::Marker* marker = markers_[irow];
	if ( marker ) 
	{
	    rc.col = cColorCol; table_->setColor( rc, marker->color() );
	    rc.col = cNameCol; table_->setText( rc, marker->name() );
	}
    }
    rc.col = cColorCol; table_->setCellReadOnly( rc, havelvl );
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


void uiMarkerDlg::getMarkerSet( Well::MarkerSet& markers ) const
{
    deepErase( markers );

    const float zfac = unitfld_->getBoolValue() ? 1 : mFromFeetFactor;
    const int nrrows = getNrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	float z = table_->getfValue( RowCol(idx,cDepthCol) );
	if ( mIsUdf(z) )
	    continue;
	z *= zfac;

	const char* txt = table_->text( RowCol(idx,cNameCol) );
	mDynamicCastGet(uiStratLevelSel*,levelsel,
			table_->getCellGroup(RowCol(idx,cLevelCol)))
	const bool havenm = txt && *txt;
	if ( !havenm )
	    continue;

	Well::Marker* marker = new Well::Marker( txt, z );
	marker->setColor( table_->getColor( RowCol(idx,cColorCol) ) );
	//TODO remove
	const int lvlid = levelsel ? levelsel->getID() : -1;
	marker->setLevelID( lvlid >= 0 ? lvlid : -1 );
	if ( stratmrkfld_->isChecked() )
	{
	    Strat::LevelSet& lvls = Strat::eLVLS();
	    const char* nm = marker->name(); const Color& col = marker->color();
	    const Strat::Level* lvl = lvls.isPresent( nm ) ? lvls.get( nm ) 
							   : lvls.add( nm, col);
	    marker->setLevelID( lvl ? lvl->id() : -1 ); 
	}

	markers += marker;
    }
}


bool uiMarkerDlg::acceptOK( CallBacker* )
{
    return true;
}
