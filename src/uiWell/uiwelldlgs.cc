/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          October 2003
 RCS:           $Id: uiwelldlgs.cc,v 1.2 2003-10-17 14:19:04 bert Exp $
________________________________________________________________________

-*/

#include "uiwelldlgs.h"
#include "uilistbox.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uitable.h"
#include "uicolor.h"
#include "uimsg.h"
#include "uibutton.h"
#include "wellimpasc.h"
#include "welldata.h"
#include "wellmarker.h"



static const char* collbls[] =
{
    "Name", "Depth", "Color", 0
};

static const int maxnrrows = 10;
static const int initnrrows = 5;

uiMarkerDlg::uiMarkerDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Well Markers","Define marker properties"))
{
    table = new uiTable( this, uiTable::Setup().rowdesc("Marker")
	    				       .rowcangrow(), "Table" );
    table->setColumnLabels( collbls );
    table->setColumnReadOnly( 2, true );
    table->setNrRows( initnrrows );
    table->rowInserted.notify( mCB(this,uiMarkerDlg,markerAdded) );
    table->leftClicked.notify( mCB(this,uiMarkerDlg,mouseClick) );

    feetfld = new uiCheckBox( this, "Depth in feet" );
    feetfld->attach( alignedBelow, table );

    markerAdded(0);
}


void uiMarkerDlg::markerAdded( CallBacker* )
{
    const int nrrows = table->nrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	BufferString labl( "Marker " );
	labl += idx+1;
	table->setRowLabel( idx, labl );
    }
}


void uiMarkerDlg::mouseClick( CallBacker* )
{
    uiTable::RowCol rc = table->notifiedCell();
    if ( rc.col != 2 ) return;

    Color newcol = table->getColor( rc );
    if ( select(newcol,this,"Marker color") )
	table->setColor( rc, newcol );
}



void uiMarkerDlg::setMarkerSet( const ObjectSet<Well::Marker>& markers )
{
    const int nrmarkers = markers.size();
    if ( !nrmarkers ) return;

    int nrrows = nrmarkers + initnrrows < maxnrrows ? nrmarkers + initnrrows 
						    : nrmarkers;
    table->setNrRows( nrrows );
    for ( int idx=0; idx<nrmarkers; idx++ )
    {
	const Well::Marker* marker = markers[idx];
	table->setText( uiTable::RowCol(idx,0), marker->name() );
	table->setValue( uiTable::RowCol(idx,1), marker->dah );
	table->setColor( uiTable::RowCol(idx,2), marker->color );
    }

    markerAdded(0);
}


void uiMarkerDlg::getMarkerSet( ObjectSet<Well::Marker>& markers ) const
{
    deepErase( markers );

    const float zfac = feetfld->isChecked() ? 0.3048 : 1;
    const int nrrows = table->nrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	const char* name = table->text( uiTable::RowCol(idx,0) );
	if ( !name || !*name ) continue;

	Well::Marker* marker = new Well::Marker( name );
	marker->dah = table->getfValue( uiTable::RowCol(idx,1) ) * zfac;
	marker->color = table->getColor( uiTable::RowCol(idx,2) );
	markers += marker;
    }
}



// ==================================================================

static const char* lasfilefilt = "*.las;;*.LAS;;*.txt;;*";
static const float defundefval = -999.25;


uiLoadLogsDlg::uiLoadLogsDlg( uiParent* p, Well::Data& wd_ )
    : uiDialog(p,uiDialog::Setup("Logs","Define log parameters",""))
    , wd(wd_)
{
    lasfld = new uiFileInput( this, "Input (pseudo-)LAS logs file",
			      uiFileInput::Setup().filter(lasfilefilt)
			      			  .withexamine() );
    lasfld->setDefaultSelectionDir( GetDataDir() );
    lasfld->valuechanged.notify( mCB(this,uiLoadLogsDlg,lasSel) );

    intvfld = new uiGenInput( this, "Depth interval to load",
			      FloatInpIntervalSpec(false) );
    intvfld->attach( alignedBelow, lasfld );

    udffld = new uiGenInput( this, "Undefined value in logs",
                    FloatInpSpec(defundefval));
    udffld->attach( alignedBelow, intvfld );

    logsfld = new uiLabeledListBox( this, "Select logs", true );
    logsfld->attach( alignedBelow, udffld );
}


void uiLoadLogsDlg::lasSel( CallBacker* )
{
    const char* lasfnm = lasfld->text();
    if ( !lasfnm || !*lasfnm ) return;

    Well::Data wd_; Well::AscImporter wdai( wd_ );
    Well::AscImporter::LasFileInfo lfi;
    const char* res = wdai.getLogInfo( lasfnm, lfi );
    if ( res ) { uiMSG().error( res ); return; }

    logsfld->box()->empty();
    logsfld->box()->addItems( lfi.lognms );

    udffld->setValue( lfi.undefval );
    intvfld->setValue( lfi.zrg );
}


bool uiLoadLogsDlg::acceptOK( CallBacker* )
{
    Well::AscImporter wdai( wd );
    Well::AscImporter::LasFileInfo lfi;
    lfi.undefval = udffld->getValue();
    assign( lfi.zrg, intvfld->getFInterval() );
    for ( int idx=0; idx<logsfld->box()->size(); idx++ )
    {
	if ( logsfld->box()->isSelected(idx) )
	    lfi.lognms += new BufferString( logsfld->box()->textOfItem(idx) );
    }

    const char* lasfnm = lasfld->text();
    if ( !lasfnm || !*lasfnm ) 
    { uiMSG().error("Enter valid filename"); return false; }

    const char* res = wdai.getLogs( lasfnm, lfi, false );
    if ( res ) { uiMSG().error( res ); return false; }

    return true;
}


// ==================================================================

uiLogSelDlg::uiLogSelDlg( uiParent* p, const BufferStringSet& strs )
    : uiDialog(p,uiDialog::Setup("Display Well logs",
				 "Select log",""))
{
    logsfld = new uiLabeledListBox( this, "Select Log" );
    logsfld->box()->addItems( strs );
}


int uiLogSelDlg::selectedLog() const
{
    return logsfld->box()->currentItem();
}
