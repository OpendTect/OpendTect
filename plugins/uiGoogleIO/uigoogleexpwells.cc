/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2007
-*/

static const char* rcsID = "$Id: uigoogleexpwells.cc,v 1.9 2011/11/23 11:35:55 cvsbert Exp $";

#include "uigoogleexpwells.h"
#include "googlexmlwriter.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "oddirs.h"
#include "ioobj.h"
#include "strmprov.h"
#include "survinfo.h"
#include "welltransl.h"
#include "welldata.h"
#include "welltrack.h"
#include "wellreader.h"
#include "iodirentry.h"
#include "ioman.h"
#include "latlong.h"
#include <iostream>


uiGoogleExportWells::uiGoogleExportWells( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Export Wells to KML",
				 "Specify wells to output","107.1.11") )
{
    uiLabeledListBox* llb = new uiLabeledListBox( this, "Wells", true );
    selfld_ = llb->box();

    mImplFileNameFld("wells");
    fnmfld_->attach( alignedBelow, llb );

    preFinalise().notify( mCB(this,uiGoogleExportWells,initWin) );
}


uiGoogleExportWells::~uiGoogleExportWells()
{
}


void uiGoogleExportWells::initWin( CallBacker* )
{
    IOM().to( WellTranslatorGroup::ioContext().getSelKey() );
    IODirEntryList del( IOM().dirPtr(), WellTranslatorGroup::ioContext() );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	IODirEntry* de = del[idx];
	if ( de && de->ioobj )
	{
	    selfld_->addItem( de->name() );
	    wellids_ += new MultiID( de->ioobj->key() );
	}
    }
}


bool uiGoogleExportWells::acceptOK( CallBacker* )
{
    mCreateWriter( "Wells", SI().name() );

    wrr.writeIconStyles( "wellpin", 20 );

    for ( int idx=0; idx<selfld_->size(); idx++ )
    {
	if ( !selfld_->isSelected(idx) )
	    continue;

	Well::Data wd;
	Well::Reader wllrdr( Well::IO::getMainFileName( *wellids_[idx] ), wd );
	if ( !wllrdr.getInfo() )
	    continue;

	wrr.writePlaceMark( "wellpin", wd.track().pos(0),
			    selfld_->textOfItem(idx) );
	if ( !wrr.strm().good() )
	    { wrr.close(); uiMSG().error("Error during write"); return false; }
    }

    wrr.close();
    return true;
}
