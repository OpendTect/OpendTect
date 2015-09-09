/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisegycommon.h"
#include "uidialog.h"
#include "uitextedit.h"
#include "uimsg.h"
#include "survinfo.h"
#include "od_strstream.h"


SEGY::FullSpec::FullSpec( Seis::GeomType gt, bool isvsp )
    : readopts_(gt)
    , isvsp_(isvsp)
    , rev0_(false)
    , zinfeet_(SI().depthsInFeet())
{
}


void SEGY::FullSpec::fillPar( IOPar& iop ) const
{
    iop.setYN( FilePars::sKeyForceRev0(), rev0_ );
    iop.setYN( "Z in feet", zinfeet_ );
    spec_.fillPar( iop );
    pars_.fillPar( iop );
    readopts_.fillPar( iop );
    if ( !isVSP() )
	iop.set( sKey::Geometry(), Seis::nameOf(geomType()) );
}


bool uiSEGY::displayWarnings( const BufferStringSet& warns, bool withstop )
{
    if ( warns.isEmpty() ) return true;

    uiString msg = "The operation was successful, but there %1:";
    msg.arg( warns.size() > 1 ? "were warnings" : "was a warning" );

    for ( int idx=0; idx<warns.size(); idx++ )
	msg.append("\n\n%1").arg( warns.get(idx) );

    if ( !withstop )
	{ uiMSG().warning( msg ); return true; }

    msg.append("\n\nContinue?");
    return uiMSG().askContinue( msg );
}


void uiSEGY::displayReport( uiParent* p, const IOPar& rep, const char* fnm )
{
    if ( fnm && *fnm && !rep.write(fnm,IOPar::sKeyDumpPretty()) )
	uiMSG().warning( "Cannot write report to specified file" );

    uiDialog* dlg = new uiDialog( p,
	    uiDialog::Setup(rep.name(),mNoDlgTitle,mNoHelpKey).modal(false) );
    dlg->setCtrlStyle( uiDialog::CloseOnly );
    od_ostrstream strstrm; rep.dumpPretty( strstrm );
    uiTextEdit* te = new uiTextEdit( dlg, rep.name() );
    te->setText( strstrm.result() );
    dlg->setDeleteOnClose( true ); dlg->go();
}
