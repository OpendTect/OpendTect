/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uigoogleexpwells.h"
#include "googlexmlwriter.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "oddirs.h"
#include "iodir.h"
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
#include "od_ostream.h"
#include "od_helpids.h"


uiGoogleExportWells::uiGoogleExportWells( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Export Wells to KML",
				 "Specify wells to output",
                                 mODHelpKey(mGoogleExportWellsHelpID) ) )
{
    uiLabeledListBox* llb = new uiLabeledListBox( this, "Well(s)",
					OD::ChooseAtLeastOne );
    selfld_ = llb->box();
    selfld_->chooseAll( true );

    mImplFileNameFld("wells");
    fnmfld_->attach( alignedBelow, llb );

    preFinalise().notify( mCB(this,uiGoogleExportWells,initWin) );
}


uiGoogleExportWells::~uiGoogleExportWells()
{
}


void uiGoogleExportWells::initWin( CallBacker* )
{
    const IODir iodir( WellTranslatorGroup::ioContext().getSelKey() );
    const IODirEntryList del( iodir, WellTranslatorGroup::ioContext() );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IODirEntry* de = del[idx];
	if ( de && de->ioobj_ )
	{
	    selfld_->addItem( de->name() );
	    wellids_ += new MultiID( de->ioobj_->key() );
	}
    }
}


bool uiGoogleExportWells::acceptOK( CallBacker* )
{
    mCreateWriter( "Wells", SI().name() );

    wrr.writeIconStyles( "wellpin", 20 );

    for ( int idx=0; idx<selfld_->size(); idx++ )
    {
	if ( !selfld_->isChosen(idx) )
	    continue;

	Well::Data wd;
	Well::Reader wllrdr( *wellids_[idx], wd );
	if ( !wllrdr.getInfo() )
	    continue;

	wrr.writePlaceMark( "wellpin", wd.track().pos(0),
			    selfld_->textOfItem(idx) );
	if ( !wrr.isOK() )
	    { uiMSG().error(wrr.errMsg()); return false; }
    }

    wrr.close();
    return true;
}
