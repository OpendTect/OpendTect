/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		October 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellmarkerdlg.cc,v 1.11 2009-05-28 11:59:12 cvsbert Exp $";


#include "uiwellmarkerdlg.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uistratlvlsel.h"
#include "uistrattreewin.h"
#include "stratlevel.h"
#include "uitable.h"

#include "ctxtioobj.h"
#include "filegen.h"
#include "iopar.h"
#include "pixmap.h"
#include "survinfo.h"
#include "welldata.h"
#include "wellimpasc.h"
#include "wellmarker.h"
#include "welltrack.h"



static const char* mrkrcollbls[] = { "Level", "Depth (MD)",
    				     "[Name]", "[Color]", 0 };
static const int cNrEmptyRows = 5;

static const int cLevelCol = 0;
static const int cDepthCol = 1;
static const int cNameCol  = 2;
static const int cColorCol = 3;

#define mSetSIDepthInFeetDef(zinft) \
    SI().getPars().setYN( SurveyInfo::sKeyDpthInFt(), zinft ); \
    SI().savePars()


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
    unitfld_->attach( ensureLeftOf, sb );

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


void uiMarkerDlg::setMarkerSet( const ObjectSet<Well::Marker>& markers,
				bool add )
{
    const int nrnew = markers.size();
    if ( !nrnew ) return;

    const float zfac = unitfld_->getBoolValue() ? 1 : mToFeetFactor;
    int startrow = add ? getNrRows() : 0;
    const int nrrows = nrnew + cNrEmptyRows + startrow;
    table_->setNrRows( nrrows );

    for ( int idx=0; idx<nrnew; idx++ )
    {
	int irow = startrow + idx;
	const Well::Marker* marker = markers[idx];
	uiStratLevelSel* levelsel = new uiStratLevelSel( 0, false, false );
	levelsel->selChange.notify( mCB(this,uiMarkerDlg,stratLvlChg) );
	table_->setCellGroup( RowCol(irow,cLevelCol), levelsel );
	levelsel->setID( marker->levelID() );
	table_->setValue( RowCol(irow,cDepthCol), marker->dah()*zfac );
	table_->setText( RowCol(irow,cNameCol), marker->name() );
	table_->setColor( RowCol(irow,cColorCol), marker->color() );
	updateFromLevel( irow, levelsel );
    }
    Well::Marker mrk;
    for ( int irow=startrow+nrnew; irow<nrrows; irow++ )
    {
	uiStratLevelSel* levelsel = new uiStratLevelSel( 0, false, false );
	levelsel->selChange.notify( mCB(this,uiMarkerDlg,stratLvlChg) );
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
    const Strat::Level* lvl = levelsel->selected();
    const bool havelvl = lvl && lvl->id_ >= 0;

    RowCol rc( irow, cColorCol );
    if ( havelvl )
    {
	rc.c() = cColorCol; table_->setColor( rc, lvl->color_ );
	rc.c() = cNameCol; table_->setText( rc, lvl->name() );
    }

    rc.c() = cColorCol; table_->setCellReadOnly( rc, havelvl );
    rc.c() = cNameCol; table_->setCellReadOnly( rc, havelvl );
}


class uiReadMarkerFile : public uiDialog
{
public:

uiReadMarkerFile( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import Markers",
				 "Specify Marker import",
				 "107.1.4"))
{
    fnmfld = new uiFileInput( this, "Input Ascii file",
	    		uiFileInput::Setup().withexamine(true)
					    .forread(true));
    fnmfld->setDefaultSelectionDir(
	    IOObjContext::getDataDirName(IOObjContext::WllInf) );
    istvdfld = new uiGenInput( this, "Depth (col 1) is",
	    			BoolInpSpec(false,"TVDSS","MD") );
    istvdfld->attach( alignedBelow, fnmfld );
    replfld = new uiGenInput( this, "Replace current markers",
	    		      BoolInpSpec(true) );
    replfld->attach( alignedBelow, istvdfld );
}

bool acceptOK( CallBacker* )
{
    fnm = fnmfld->fileName();
    if ( File_isEmpty(fnm) )
	{ uiMSG().error( "Invalid input file" ); return false; }
    istvd = istvdfld->getBoolValue();
    repl = replfld->getBoolValue();
    return true;
}

    BufferString	fnm;
    bool		istvd;
    bool		repl;
    uiFileInput*	fnmfld;
    uiGenInput*		istvdfld;
    uiGenInput*		replfld;

};


void uiMarkerDlg::rdFile( CallBacker* )
{
    uiReadMarkerFile dlg( this );
    if ( !dlg.go() ) return;
    Well::Data wd; wd.track() = track_;
    Well::AscImporter wdai( wd );
    const bool inft = !unitfld_->getBoolValue();
    const char* res = wdai.getMarkers( dlg.fnm, dlg.istvd, inft );
    if ( res && *res )
	uiMSG().error( res );
    else
	setMarkerSet( wd.markers(), !dlg.repl );
}


void uiMarkerDlg::getMarkerSet( ObjectSet<Well::Marker>& markers ) const
{
    deepErase( markers );

    const float zfac = unitfld_->getBoolValue() ? 1 : mFromFeetFactor;
    const int nrrows = getNrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	const float z = table_->getfValue( RowCol(idx,cDepthCol) );
	if ( mIsUdf(z) )
	    continue;

	const char* txt = table_->text( RowCol(idx,cNameCol) );
	mDynamicCastGet(uiStratLevelSel*,levelsel,
			table_->getCellGroup(RowCol(idx,cLevelCol)))
	const int lvlid = levelsel ? levelsel->getID() : -1;
	const bool havelvl = lvlid >= 0;
	const bool havenm = txt && *txt;
	if ( !havelvl && !havenm )
	    continue;

	Well::Marker* marker = lvlid >= 0 ? new Well::Marker( lvlid, z )
					  : new Well::Marker( txt, z );
	if ( !havelvl )
	    marker->setColor( table_->getColor( RowCol(idx,cColorCol) ) );
	markers += marker;
    }
}


bool uiMarkerDlg::acceptOK( CallBacker* )
{
    mSetSIDepthInFeetDef( !unitfld_->getBoolValue() );
    return true;
}
