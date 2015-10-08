/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Feb 2004 / Sep 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisegysip.h"
#include "uisegyreadstarter.h"
#include "uisegyreadfinisher.h"



uiDialog* uiSEGYSurvInfoProvider::dialog( uiParent* p )
{
    return new uiSEGYReadStarter( p, true );
}

#define mShowErr(s) \
    uiMainWin* mw = uiMSG().setMainWin( uiMain::theMain().topLevel() ); \
    uiMSG().error(s); \
    uiMSG().setMainWin(mw);


bool uiSEGYSurvInfoProvider::getInfo( uiDialog* d, TrcKeyZSampling& cs,
				      Coord crd[3] )
{
    imppars_.setEmpty();
    if ( !d )
	return false;
    mDynamicCastGet(uiSEGYReadStarter*,rdst,d)
    if ( !rdst )
	{ pErrMsg("Huh?"); return false; }
    else if ( rdst->uiResult() != 1 )
	return false; // canceled
    else if ( !rdst->getInfo4SI(cs,crd) )
	return false;

    xyinft_ = rdst->zInFeet();
    const SEGY::FullSpec fullspec( rdst->fullSpec() );
    fullspec.fillPar( imppars_ );
    userfilename_ = rdst->userFileName();
    return true;
}


void uiSEGYSurvInfoProvider::startImport( uiParent* p, const IOPar& iop )
{
    Seis::GeomType gt = Seis::Vol; Seis::getFromPar( iop, gt );
    SEGY::FullSpec fullspec( gt );
    fullspec.usePar( iop );
    uiSEGYReadFinisher dlg( p, fullspec, userfilename_ );
    if ( dlg.go() )
	dlg.setAsDefaultObj();
}


const char* uiSEGYSurvInfoProvider::importAskQuestion() const
{
    return "Import the SEG-Y data used for survey setup now?";
}
