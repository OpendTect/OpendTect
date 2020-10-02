/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2015
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uisegyexamine.h"
#include "uidialog.h"
#include "uitextedit.h"
#include "uisettings.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "segytr.h"
#include "survinfo.h"
#include "settings.h"
#include "od_strstream.h"
#include "keystrs.h"

static const char* sKeyZInFeet = "Z in feet";
static const char* sKeyIsVSP = "Is Zero-offset VSP";
static const char* sKeySuppress = "SEGY.Suppress Warnings";
static const char* sKeyMaxReasNrSamps = "SEGY.Max Reasonable Nr Samples";


int SEGY::cMaxReasonableNrSamples()
{
    int maxreasnrsamps = 25000;
    Settings::common().get( sKeyMaxReasNrSamps, maxreasnrsamps );
    return maxreasnrsamps;
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


bool uiSEGY::displayWarnings( uiParent* p, const uiStringSet& inpwarns,
			      bool withstop, int nrskipped )
{
    uiStringSet warns( inpwarns );
    if ( nrskipped > 0 )
	warns += od_static_tr("uiSEGY_displayWarnings",
		    "During import, %1 traces were rejected").arg( nrskipped );
    if ( warns.isEmpty() )
	return true;

    TypeSet<int> suppress;
    Settings::common().get( sKeySuppress, suppress );

    uiString msg = od_static_tr("uiSEGY_displayWarnings",
		    "The operation was successful, but:");

    TypeSet<int> curwarnnrs;
    for ( int idx=0; idx<warns.size(); idx++ )
    {
	BufferString curwarn( toString(warns.get(idx)) );
	char* nrptr = curwarn.getCStr() + 1;
	char* msgptr = curwarn.getCStr() + 2;
	*msgptr = '\0'; msgptr += 2;
	const int msgnr = toInt( nrptr );
	if ( suppress.isPresent(msgnr) )
	    continue;

	curwarnnrs += msgnr;
	msg.appendPhrase( warns.get(idx) );
    }

    if ( curwarnnrs.isEmpty() ) // all suppressed
	return true;

    bool suppresscurwarns = false;
    bool res = true;
    if ( !withstop )
	suppresscurwarns = gUiMsg(p).warning( msg, uiString::empty(),
					    uiString::empty(), true );
    else
    {
	msg.appendPhrase( od_static_tr("displayWarnings","Continue?") );
	res = gUiMsg(p).askGoOn( msg, true, &suppresscurwarns );
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
	gUiMsg(p).warning( uiStrings::phrCannotWrite(
	  od_static_tr("displayReport",("report to specified file"))) );

    uiDialog* dlg = new uiDialog( p,
	    uiDialog::Setup(toUiString(rep.name()),
					mNoDlgTitle,mNoHelpKey).modal(false) );
    dlg->setCtrlStyle( uiDialog::CloseOnly );
    od_ostrstream strstrm; rep.dumpPretty( strstrm );
    uiTextEdit* te = new uiTextEdit( dlg, rep.name() );
    te->setText( strstrm.result() );
    dlg->setDeleteOnClose( true ); dlg->go();
}



class uiSEGYSettingsGroup : public uiSettingsGroup
{ mODTextTranslationClass(uiSEGYSettingsGroup);
public:

    mDecluiSettingsGroupPublicFns( uiSEGYSettingsGroup,
				   General, "SEGY", "segy",
				   toUiString("SEG-Y"), mTODOHelpKey )

uiSEGYSettingsGroup( uiParent* p, Settings& setts )
    : uiSettingsGroup(p,setts)
    , initialsuppresswarnings_(setts.isTrue(sKeySuppress))
    , initialmaxreassamps_(SEGY::cMaxReasonableNrSamples())
    , initialexaminenrtrcs_(uiSEGYExamine::Setup::getDefNrTrcs())
    , initialebcdic_(setts.isTrue(SEGYSeisTrcTranslator::sKeyHdrEBCDIC()))
{
    suppressfld_ = new uiGenInput( this, tr("Suppress SEG-Y warnings"),
			  BoolInpSpec(initialsuppresswarnings_) );
    maxnrsampfld_ = new uiGenInput( this,
		tr("Maximum acceptable number of samples per trace"),
			  IntInpSpec(initialmaxreassamps_,1,32768) );
    maxnrsampfld_->attach( alignedBelow, suppressfld_ );
    uiString tt( tr("Up to 32k samples is supported by SEG-Y") );
    tt.appendPhrase( tr("But very large numbers usually indicate a problem "
			"(rather than a real huge trace length)" ) );
    maxnrsampfld_->setToolTip( tt );
    examinenrtrcsfld_ = new uiGenInput( this,
		tr("Default number of traces to examine"),
			  IntInpSpec(initialexaminenrtrcs_,1,mUdf(int)) );
    examinenrtrcsfld_->attach( alignedBelow, maxnrsampfld_ );
    asctxtfld_ = new uiGenInput( this, tr("Output Textual Header encoding"),
			  BoolInpSpec(!initialebcdic_,
			  tr("ASCII (recommended)"),tr("EBCDIC (legacy)") ) );
    asctxtfld_->attach( alignedBelow, examinenrtrcsfld_ );

    bottomobj_ = asctxtfld_;
}


void doCommit( uiRetVal& )
{
    updateSettings( initialsuppresswarnings_, suppressfld_->getBoolValue(),
		    sKeySuppress );
    updateSettings( initialmaxreassamps_, maxnrsampfld_->getIntValue(),
		    sKeyMaxReasNrSamps );
    updateSettings( initialexaminenrtrcs_, examinenrtrcsfld_->getIntValue(),
		    sKeySettNrTrcExamine );
    updateSettings( initialebcdic_, !asctxtfld_->getBoolValue(),
		    SEGYSeisTrcTranslator::sKeyHdrEBCDIC() );
}

    const bool	initialsuppresswarnings_;
    const bool	initialebcdic_;
    const int	initialmaxreassamps_;
    const int	initialexaminenrtrcs_;

    uiGenInput*	suppressfld_;
    uiGenInput*	maxnrsampfld_;
    uiGenInput*	examinenrtrcsfld_;
    uiGenInput*	asctxtfld_;

};

#include "uisegywriteopts.h"
#include "uisegydirectinserter.h"

void uiSEGY::initClasses()
{
    uiSEGYSettingsGroup::initClass();

    uiSEGYDirectVolOpts::initClass();
    uiSEGYDirectPS3DOpts::initClass();
    uiSEGYDirectVolInserter::initClass();
    uiSEGYDirect2DInserter::initClass();
    uiSEGYDirectPS3DInserter::initClass();
}
