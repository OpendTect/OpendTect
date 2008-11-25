/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		October 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellmarkerdlg.cc,v 1.6 2008-11-25 15:35:26 cvsbert Exp $";


#include "uiwellmarkerdlg.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uistratlvlsel.h"
#include "uitable.h"

#include "ctxtioobj.h"
#include "filegen.h"
#include "iopar.h"
#include "survinfo.h"
#include "welldata.h"
#include "wellimpasc.h"
#include "wellmarker.h"
#include "welltrack.h"



static const char* mrkrcollbls[] = { "Name", "Depth (MD)", "Color",
    				     "Tied to level", 0 };
static const int sNrEmptyRows = 5;

static const int sNameCol  = 0;
static const int sDepthCol = 1;
static const int sColorCol = 2;
static const int sLevelCol = 3;

#define mFromFeetFac 0.3048
#define mSetSIDepthInFeetDef(zinft) \
    SI().getPars().setYN( SurveyInfo::sKeyDpthInFt, zinft ); \
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
    table_->setColumnReadOnly( sColorCol, true );
    table_->setColumnResizeMode( uiTable::ResizeToContents );
    table_->setColumnStretchable( sLevelCol, true );
    table_->setNrRows( sNrEmptyRows );
    table_->leftClicked.notify( mCB(this,uiMarkerDlg,mouseClick) );

    BoolInpSpec mft( !SI().depthsInFeetByDefault(), "Meter", "Feet" );
    unitfld_ = new uiGenInput( this, "Depth unit", mft );
    unitfld_->attach( leftAlignedBelow, table_ );

    uiButton* rfbut = new uiPushButton( this, "&Read file",
	    				mCB(this,uiMarkerDlg,rdFile), false );
    rfbut->attach( rightTo, unitfld_ ); rfbut->attach( rightBorder );
}


int uiMarkerDlg::getNrRows() const
{
    for ( int idx=table_->nrRows()-1; idx>=0; idx-- )
    {
	const char* txt = table_->text( RowCol(idx,sNameCol) );
	if ( txt && *txt ) return idx+1;
    }
    return 0;
}


void uiMarkerDlg::mouseClick( CallBacker* )
{
    RowCol rc = table_->notifiedCell();
    if ( rc.col != sColorCol ) return;

    Color newcol = table_->getColor( rc );
    if ( selectColor(newcol,this,"Marker color") )
	table_->setColor( rc, newcol );
}


void uiMarkerDlg::setMarkerSet( const ObjectSet<Well::Marker>& markers,
				bool add )
{
    const int nrnew = markers.size();
    if ( !nrnew ) return;

    const float zfac = unitfld_->getBoolValue() ? 1 : 1./mFromFeetFac;
    int startrow = add ? getNrRows() : 0;
    const int nrrows = nrnew + sNrEmptyRows + startrow;
    table_->setNrRows( nrrows );

    for ( int idx=0; idx<nrnew; idx++ )
    {
	int irow = startrow + idx;
	const Well::Marker* marker = markers[idx];
	table_->setText( RowCol(irow,sNameCol), marker->name() );
	table_->setValue( RowCol(irow,sDepthCol), marker->dah_*zfac );
	table_->setColor( RowCol(irow,sColorCol), marker->color_ );
	uiStratLevelSel* levelsel = new uiStratLevelSel( 0, false, false );
	levelsel->levelChanged.notify( mCB(this,uiMarkerDlg,stratLvlChg) );
	table_->setCellGroup( RowCol(irow,sLevelCol), levelsel );
	levelsel->setLevelID( marker->stratlevelid_ );
    }
    Well::Marker mrk;
    for ( int irow=startrow+nrnew; irow<nrrows; irow++ )
    {
	table_->setText( RowCol(irow,sNameCol), "" );
	table_->setText( RowCol(irow,sDepthCol), "" );
	table_->setColor( RowCol(irow,sColorCol), mrk.color_ );
	uiStratLevelSel* levelsel = new uiStratLevelSel( 0, false, false );
	levelsel->levelChanged.notify( mCB(this,uiMarkerDlg,stratLvlChg) );
	table_->setCellGroup( RowCol(irow,sLevelCol), levelsel );
    }
}


void uiMarkerDlg::stratLvlChg( CallBacker* cb )
{
    mDynamicCastGet(uiStratLevelSel*,levelsel,cb)
    const Color* col = levelsel ? levelsel->getLevelColor() : 0;
    if ( !col ) return;

    const RowCol rc = table_->getCell( levelsel );
    table_->setColor( RowCol(rc.row,sColorCol), *col );
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

    const float zfac = unitfld_->getBoolValue() ? 1 : mFromFeetFac;
    const int nrrows = getNrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	const char* txt = table_->text( RowCol(idx,sNameCol) );
	if ( !txt || !*txt ) continue;

	Well::Marker* marker = new Well::Marker( txt );
	marker->dah_ = table_->getfValue( RowCol(idx,sDepthCol) ) * zfac;
	marker->color_ = table_->getColor( RowCol(idx,sColorCol) );

	mDynamicCastGet(uiStratLevelSel*,levelsel,
			table_->getCellGroup(RowCol(idx,sLevelCol)))
	marker->stratlevelid_ = levelsel ? levelsel->getLevelID() : -1;
	markers += marker;
    }
}


bool uiMarkerDlg::acceptOK( CallBacker* )
{
    mSetSIDepthInFeetDef( !unitfld_->getBoolValue() );
    return true;
}
