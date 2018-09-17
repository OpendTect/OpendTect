/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2004 / Sep 2015
________________________________________________________________________

-*/

#include "uisegysip.h"
#include "uisegyreadstarter.h"
#include "uisegyreadfinisher.h"
#include "zdomain.h"



uiDialog* uiSEGYSurvInfoProvider::dialog( uiParent* p, TDInfo tdinf )
{
    uiSEGYReadStarter* ret = new uiSEGYReadStarter( p, true );
    ret->setZIsTime( tdinf == uiSurvInfoProvider::Time );
    tdinfoknown_ = false;
    return ret;
}


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


    int itdinf = (int)tdinfo_;
    rdst->getSIPInfo( xyinft_, itdinf, tdinfoknown_, imppars_, userfilename_ );
    tdinfo_ = (uiSurvInfoProvider::TDInfo)itdinf;
    return true;
}


uiSurvInfoProvider::TDInfo uiSEGYSurvInfoProvider::tdInfo( bool& isknown ) const
{
    isknown = tdinfoknown_;
    return tdinfo_;
}


void uiSEGYSurvInfoProvider::startImport( uiParent* p, const IOPar& iop )
{
    Seis::GeomType gt = Seis::Vol; Seis::getFromPar( iop, gt );
    SEGY::FullSpec fullspec( gt );
    fullspec.usePar( iop );
    uiSEGYReadFinisher dlg( p, fullspec, userfilename_, ZDomain::isTime(iop) );
    if ( dlg.go() )
	dlg.setAsDefaultObj();
}


uiString uiSEGYSurvInfoProvider::importAskQuestion() const
{
    return uiStrings::phrImport(
			  tr("the SEG-Y data used for survey setup now?"));
}

