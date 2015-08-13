/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          July 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: $";

#include "uisegyreadfinisher.h"
#include "uimsg.h"


uiString uiSEGYReadFinisher::getWinTile( const FullSpec& fs )
{
    const Seis::GeomType gt = fs.geomType();
    const bool isvsp = fs.isVSP();

    uiString ret;
    if ( fs.spec_.nrFiles() > 1 && !isvsp && Seis::is2D(gt) )
	ret = tr("Import %1s");
    else
	ret = tr("Import %1");

    if ( isvsp )
	ret.arg( tr("Zero-offset VSP") );
    else
	ret.arg( Seis::nameOf(gt) );
    return ret;
}


uiString uiSEGYReadFinisher::getDlgTitle( const char* usrspec )
{
    uiString ret( "Importing %1" );
    ret.arg( usrspec );
    return ret;
}


#include "uilabel.h"

uiSEGYReadFinisher::uiSEGYReadFinisher( uiParent* p, const FullSpec& fs,
					const char* usrspec )
    : uiDialog(p,uiDialog::Setup(getWinTile(fs),getDlgTitle(usrspec),
				  mTODOHelpKey ) )
    , fs_(fs)
{
    new uiLabel( this, "TODO: implement" );

    postFinalise().notify( mCB(this,uiSEGYReadFinisher,initWin) );
}


uiSEGYReadFinisher::~uiSEGYReadFinisher()
{
}


void uiSEGYReadFinisher::initWin( CallBacker* )
{
}


bool uiSEGYReadFinisher::acceptOK( CallBacker* )
{
    uiMSG().error( "TODO: omplement actual import" );
    return false;
}
