/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		October 2003
 RCS:		$Id: uiwellmarkerdlg.cc,v 1.2 2008-02-20 04:44:43 cvsnanne Exp $
________________________________________________________________________

-*/


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
static const int nremptyrows = 5;

#define mFromFeetFac 0.3048
#define mSetSIDepthInFeetDef(zinft) \
    SI().getPars().setYN( SurveyInfo::sKeyDpthInFt, zinft ); \
    SI().savePars()


uiMarkerDlg::uiMarkerDlg( uiParent* p, const Well::Track& t )
	: uiDialog(p,uiDialog::Setup("Well Markers",
				     "Define marker properties",
				     "107.1.1"))
    	, track(t)
{
    table = new uiTable( this, uiTable::Setup().rowdesc("Marker")
	    				       .rowgrow(true) 
					       .defrowlbl("") );
    table->setColumnLabels( mrkrcollbls );
    table->setColumnReadOnly( 2, true );
    table->setNrRows( nremptyrows );
    table->leftClicked.notify( mCB(this,uiMarkerDlg,mouseClick) );

    BoolInpSpec mft( !SI().depthsInFeetByDefault(), "Meter", "Feet" );
    unitfld = new uiGenInput( this, "Depth unit", mft );
    unitfld->attach( leftAlignedBelow, table );

    uiButton* rfbut = new uiPushButton( this, "&Read file",
	    				mCB(this,uiMarkerDlg,rdFile), false );
    rfbut->attach( rightTo, unitfld ); rfbut->attach( rightBorder );
}


int uiMarkerDlg::getNrRows() const
{
    for ( int idx=table->nrRows()-1; idx>=0; idx-- )
    {
	const char* txt = table->text( RowCol(idx,0) );
	if ( txt && *txt ) return idx+1;
    }
    return 0;
}


void uiMarkerDlg::mouseClick( CallBacker* )
{
    RowCol rc = table->notifiedCell();
    if ( rc.col != 2 ) return;

    Color newcol = table->getColor( rc );
    if ( selectColor(newcol,this,"Marker color") )
	table->setColor( rc, newcol );
}


void uiMarkerDlg::setMarkerSet( const ObjectSet<Well::Marker>& markers,
				bool add )
{
    const int nrnew = markers.size();
    if ( !nrnew ) return;

    const float zfac = unitfld->getBoolValue() ? 1 : 1./mFromFeetFac;
    int startrow = add ? getNrRows() : 0;
    const int nrrows = nrnew + nremptyrows + startrow;
    table->setNrRows( nrrows );

    for ( int idx=0; idx<nrnew; idx++ )
    {
	int irow = startrow + idx;
	const Well::Marker* marker = markers[idx];
	table->setText( RowCol(irow,0), marker->name() );
	table->setValue( RowCol(irow,1), marker->dah*zfac );
	table->setColor( RowCol(irow,2), marker->color );
	uiStratLevelSel* levelsel = new uiStratLevelSel( 0, false, false );
	levelsel->levelChanged.notify( mCB(this,uiMarkerDlg,stratLvlChg) );
	table->setCellObject( RowCol(irow,3), levelsel->attachObj() );
    }
    Well::Marker mrk;
    for ( int irow=startrow+nrnew; irow<nrrows; irow++ )
    {
	table->setText( RowCol(irow,0), "" );
	table->setText( RowCol(irow,1), "" );
	table->setColor( RowCol(irow,2), mrk.color );
	uiStratLevelSel* levelsel = new uiStratLevelSel( 0, false, false );
	levelsel->levelChanged.notify( mCB(this,uiMarkerDlg,stratLvlChg) );
	table->setCellObject( RowCol(irow,3), levelsel->attachObj() );
    }
}


void uiMarkerDlg::stratLvlChg( CallBacker* cb )
{
    mDynamicCastGet(uiStratLevelSel*,levelsel,cb)
    const Color* col = levelsel ? levelsel->getLvlColor() : 0;
    if ( !col ) return;

    const RowCol rc = table->getCell( levelsel->attachObj() );
    table->setColor( RowCol(rc.row,2), *col );
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
	    			BoolInpSpec(true,"TVDSS","MD") );
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
    Well::Data wd; wd.track() = track;
    Well::AscImporter wdai( wd );
    const bool inft = !unitfld->getBoolValue();
    const char* res = wdai.getMarkers( dlg.fnm, dlg.istvd, inft );
    if ( res && *res )
	uiMSG().error( res );
    else
	setMarkerSet( wd.markers(), !dlg.repl );
}


void uiMarkerDlg::getMarkerSet( ObjectSet<Well::Marker>& markers ) const
{
    deepErase( markers );

    const float zfac = unitfld->getBoolValue() ? 1 : mFromFeetFac;
    const int nrrows = getNrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	const char* txt = table->text( RowCol(idx,0) );
	if ( !txt || !*txt ) continue;

	Well::Marker* marker = new Well::Marker( txt );
	marker->dah = table->getfValue( RowCol(idx,1) ) * zfac;
	marker->color = table->getColor( RowCol(idx,2) );
	markers += marker;
    }
}


bool uiMarkerDlg::acceptOK( CallBacker* )
{
    mSetSIDepthInFeetDef(!unitfld->getBoolValue());
    return true;
}
