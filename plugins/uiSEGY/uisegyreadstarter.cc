/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          July 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: $";

#include "uisegyreadstarter.h"

#include "uisegydef.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "survinfo.h"
#include "seistype.h"


uiSEGYReadStarter::uiSEGYReadStarter( uiParent* p, const char* fnm )
    : uiDialog(p,uiDialog::Setup(tr("Import SEG-Y Data"),mNoDlgTitle,
				  mTODOHelpKey ) )
    , geomtype_(Seis::Vol)
    , isvsp_(false)
{
    uiFileInput::Setup fisu( uiFileDialog::Gen );
    fisu.filter( uiSEGYFileSpec::fileFilter() ).forread( true );
    inpfld_ = new uiFileInput( this, "Input file (multiple use wildcard)", fisu );
    inpfld_->valuechanged.notify( mCB(this,uiSEGYReadStarter,inpSel) );

    uiStringSet inps;
    if ( SI().has3D() )
    {
	addTyp( inps, (int)Seis::Vol );
	addTyp( inps, (int)Seis::VolPS );
    }
    if ( SI().has2D() )
    {
	addTyp( inps, (int)Seis::Line );
	addTyp( inps, (int)Seis::LinePS );
    }
    addTyp( inps, -1 );
    typfld_ = new uiGenInput( this, tr("Data contained"),
	    			StringListInpSpec(inps) );
    typfld_->attach( alignedBelow, inpfld_ );
}


void uiSEGYReadStarter::addTyp( uiStringSet& inps, int typ )
{
    inptyps_ += typ;
    if ( typ < 0 )
	inps += tr( "Zero-offset VSP" );
    else if ( typ == (int)Seis::Vol )
	inps += tr( "3D seismic data" );
    else if ( typ == (int)Seis::VolPS )
	inps += tr( "3D PreStack data" );
    else if ( typ == (int)Seis::Line )
	inps += tr( "2D Seismic data" );
    else if ( typ == (int)Seis::LinePS )
	inps += tr( "2D PreStack data" );
    else
	{ pErrMsg( "Huh" ); }
}


uiSEGYReadStarter::~uiSEGYReadStarter()
{
}


void uiSEGYReadStarter::inpSel( CallBacker* )
{
}


bool uiSEGYReadStarter::acceptOK( CallBacker* )
{
    uiMSG().error( "TODO: implement" );
    return false;
}
