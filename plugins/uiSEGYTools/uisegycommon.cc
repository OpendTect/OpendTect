/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2015
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"
#include "uitextedit.h"
#include "uimsg.h"
#include "survinfo.h"
#include "settings.h"
#include "od_strstream.h"

static const char* sKeyZInFeet = "Z in feet";
static const char* sKeyIsVSP = "Is Zero-offset VSP";
static const char* sKeySuppress = "SEGY.Suppress Warnings";
static const char* sKeyMaxReasNrSamps = "SEGY.Max Reasonable Nr Samples";
static int maxreasnrsamps_ = -1;


int SEGY::cMaxReasonableNrSamples()
{
    if ( maxreasnrsamps_ < 0 )
    {
	Settings::common().get( sKeyMaxReasNrSamps, maxreasnrsamps_ );
	if ( maxreasnrsamps_ < 1 )
	    maxreasnrsamps_ = 100000;
    }
    return maxreasnrsamps_;
}


SEGY::FullSpec::FullSpec( Seis::GeomType gt, bool isvsp )
    : readopts_(gt)
    , isvsp_(isvsp)
    , rev_(-1)
    , zinfeet_(SI().depthsInFeet())
{
}


void SEGY::FullSpec::fillPar( IOPar& iop ) const
{
    spec_.fillPar( iop );
    pars_.fillPar( iop );
    readopts_.fillPar( iop );
    iop.set( FilePars::sKeyRevision(), rev_ );
    iop.setYN( FilePars::sKeyForceRev0(), rev_ == 0 );
    iop.setYN( sKeyIsVSP, isvsp_ );
    iop.setYN( sKeyZInFeet, zinfeet_ );
    if ( !isVSP() )
	iop.set( sKey::Geometry(), Seis::nameOf(geomType()) );
    else
	iop.removeWithKey( sKey::Geometry() );
}


void SEGY::FullSpec::usePar( const IOPar& iop )
{
    Seis::GeomType gt = Seis::Vol; Seis::getFromPar( iop, gt );
    spec_.usePar( iop );
    pars_.usePar( iop );
    readopts_.setGeomType( gt );
    readopts_.usePar( iop );
    iop.getYN( sKeyIsVSP, isvsp_ );
    iop.getYN( sKeyZInFeet, zinfeet_ );
    iop.get( FilePars::sKeyRevision(), rev_ );
    if ( iop.isTrue(FilePars::sKeyForceRev0()) )
	rev_ = 0;
}


bool uiSEGY::displayWarnings( const BufferStringSet& warns, bool withstop )
{
    if ( warns.isEmpty() )
	return true;

    TypeSet<int> suppress;
    Settings::common().get( sKeySuppress, suppress );

    uiString msg = toUiString("The operation was successful, but there %1:");
    msg.arg( warns.size() > 1 ? "were warnings" : "was a warning" );

    TypeSet<int> curwarnnrs;
    for ( int idx=0; idx<warns.size(); idx++ )
    {
	BufferString curwarn( warns.get(idx) );
	char* nrptr = curwarn.getCStr() + 1;
	char* msgptr = curwarn.getCStr() + 2;
	*msgptr = '\0'; msgptr += 2;
	const int msgnr = toInt( nrptr );
	if ( suppress.isPresent(msgnr) )
	    continue;

	curwarnnrs += msgnr;
	msg.append("\n\n%1").arg( msgptr );
    }

    if ( curwarnnrs.isEmpty() ) // all suppressed
	return true;

    bool suppresscurwarns = false;
    bool res = true;
    if ( !withstop )
	suppresscurwarns = uiMSG().warning( msg, uiString::emptyString(),
					    uiString::emptyString(), true );
    else
    {
	msg.append("\n\nContinue?");
	res = uiMSG().askGoOn( msg, true, &suppresscurwarns );
    }

    if ( suppresscurwarns )
    {
	suppress.createUnion( curwarnnrs );
	Settings::common().set( sKeySuppress, suppress );
	Settings::common().write();
    }

    return res;
}


void uiSEGY::displayReport( uiParent* p, const IOPar& rep, const char* fnm )
{
    if ( fnm && *fnm && !rep.write(fnm,IOPar::sKeyDumpPretty()) )
	uiMSG().warning( toUiString("Cannot write report to specified file") );

    const uiString caption = toUiString( rep.name() );
    uiDialog* dlg = new uiDialog( p,
	    uiDialog::Setup(caption,mNoDlgTitle,mNoHelpKey).modal(false) );
    dlg->setCtrlStyle( uiDialog::CloseOnly );
    od_ostrstream strstrm; rep.dumpPretty( strstrm );
    uiTextEdit* te = new uiTextEdit( dlg, rep.name() );
    te->setText( strstrm.result() );
    dlg->setDeleteOnClose( true ); dlg->go();
}


#include "uisegywriteopts.h"
#include "uisegydirectinserter.h"

void uiSEGY::initClasses()
{
    uiSEGYDirectVolOpts::initClass();
    uiSEGYDirectPS3DOpts::initClass();
    uiSEGYDirectVolInserter::initClass();
    uiSEGYDirect2DInserter::initClass();
    uiSEGYDirectPS3DInserter::initClass();
}
