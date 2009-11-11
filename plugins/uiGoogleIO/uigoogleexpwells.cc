/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2007
-*/

static const char* rcsID = "$Id";

#include "uigoogleexpwells.h"
#include "odgooglexmlwriter.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "oddirs.h"
#include "ioobj.h"
#include "strmprov.h"
#include "survinfo.h"
#include "welltransl.h"
#include "welldata.h"
#include "wellreader.h"
#include "iodirentry.h"
#include "ioman.h"
#include "latlong.h"
#include <iostream>


uiGoogleExportWells::uiGoogleExportWells( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Export Wells to KML",
				 "Specify wells to output","0.3.10") )
{
    uiLabeledListBox* llb = new uiLabeledListBox( this, "Wells", true );
    selfld_ = llb->box();

    fnmfld_ = new uiFileInput( this, "Output file",
		uiFileInput::Setup(uiFileDialog::Gen,GetBaseDataDir())
		.forread(false).filter("*.kml") );
    fnmfld_->attach( alignedBelow, llb );

    finaliseStart.notify( mCB(this,uiGoogleExportWells,initWin) );
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



#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGoogleExportWells::acceptOK( CallBacker* )
{
    const BufferString fnm( fnmfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet("Please enter a file name" )

    ODGoogle::XMLWriter wrr( "Wells", fnm, SI().name() );
    if ( !wrr.isOK() )
	mErrRet(wrr.errMsg())

    wrr.writeIconStyles( "wellpin" );

    Well::Data wd;
    for ( int idx=0; idx<selfld_->size(); idx++ )
    {
	if ( !selfld_->isSelected(idx) )
	    continue;

	Well::Reader wllrdr( Well::IO::getMainFileName( *wellids_[idx] ), wd );
	if ( !wllrdr.getInfo() )
	    continue;

	wrr.writePlaceMark( "wellpin", wd.info().surfacecoord,
			    selfld_->textOfItem(idx) );
	if ( !wrr.strm().good() )
	    { wrr.close(); mErrRet("Error during write"); }
    }

    wrr.close();
    return true;
}
