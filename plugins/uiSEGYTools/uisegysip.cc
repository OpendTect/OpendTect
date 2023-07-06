/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegysip.h"

#include "uisegymultilinesel.h"
#include "uisegyreadstarter.h"
#include "uisegyreadfinisher.h"
#include "uisegysipclassic.h"
#include "uisurvinfoed.h"

#include "envvars.h"
#include "segyhdrdef.h"


void uiSEGYToolsInitSIP()
{
    uiSurveyInfoEditor::addInfoProvider(new uiSEGYSurvInfoProvider);
    if ( GetEnvVarYN( "OD_ENABLE_SEGY_CLASSIC" ) )
	uiSurveyInfoEditor::addInfoProvider(new uiSEGYClassicSurvInfoProvider);
}


uiSEGYSurvInfoProvider::uiSEGYSurvInfoProvider()
    : uiSurvInfoProvider()
{}

uiSEGYSurvInfoProvider::~uiSEGYSurvInfoProvider()
{}


uiDialog* uiSEGYSurvInfoProvider::dialog( uiParent* p )
{
    return new uiSEGYReadStarter( p, true );
}

#define mShowErr(s) \
    uiMainWin* mw = uiMSG().setMainWin( uiMain::instance().topLevel() ); \
    uiMSG().error(s); \
    uiMSG().setMainWin(mw);


bool uiSEGYSurvInfoProvider::getInfo( uiDialog* d, TrcKeyZSampling& tkzs,
				      Coord crd[3] )
{
    imppars_.setEmpty();
    if ( !d )
	return false;

    mDynamicCastGet(uiSEGYReadStarter*,rdst,d)
    if ( !rdst )
	{ pErrMsg("Huh?"); return false; }
    else if ( rdst->uiResult() != (int)uiDialog::Accepted )
	return false; // canceled
    else if ( !rdst->getInfo4SI(tkzs,crd) )
	return false;

    coordsystem_ = rdst->getCoordSystem();
    xyinft_ = rdst->zInFeet();
    const SEGY::FullSpec fullspec( rdst->fullSpec() );
    fullspec.fillPar( imppars_ );
    userfilename_ = rdst->userFileName();
    return true;
}


static void addBytePars( IOPar& par, const IOPar& imppars, const char* key )
{
    int byte = -1;
    if ( !imppars.get(key,byte) )
	return;

    byte++;
    par.set( key, byte );
}


void uiSEGYSurvInfoProvider::fillLogPars( IOPar& par ) const
{
    uiSurvInfoProvider::fillLogPars( par );
    par.set( sKey::CrFrom(), userfilename_ );
    addBytePars( par, imppars_, SEGY::TrcHeaderDef::sInlByte() );
    addBytePars( par, imppars_, SEGY::TrcHeaderDef::sCrlByte() );
    addBytePars( par, imppars_, SEGY::TrcHeaderDef::sXCoordByte() );
    addBytePars( par, imppars_, SEGY::TrcHeaderDef::sYCoordByte() );
}


IOPar* uiSEGYSurvInfoProvider::getCoordSystemPars() const
{
    if ( !coordsystem_ )
	return nullptr;

    auto* crspar = new IOPar;
    coordsystem_->fillPar( *crspar );
    return crspar;
}


IOPar* uiSEGYSurvInfoProvider::getImportPars() const
{
    return imppars_.isEmpty() ? nullptr : new IOPar(imppars_);
}


void uiSEGYSurvInfoProvider::startImport( uiParent* p, const IOPar& iop )
{
    Seis::GeomType gt = Seis::Vol; Seis::getFromPar( iop, gt );
    SEGY::FullSpec fullspec( gt );
    fullspec.usePar( iop );
    if ( gt == Seis::Line && fullspec.spec_.nrFiles() > 1 &&
	 fullspec.linenames_.size() != fullspec.spec_.nrFiles() )
    {
	int wcidx = 0;
	uiSEGYMultiLineSel linedlg( p, fullspec.spec_, wcidx,
			   fullspec.linenames_ );
	if ( !linedlg.go() )
	    return;
    }

    uiSEGYReadFinisher dlg( p, fullspec, userfilename_ );
    if ( dlg.go() )
	dlg.setAsDefaultObj();
}


const char* uiSEGYSurvInfoProvider::importAskQuestion() const
{
    return "Import the SEG-Y data used for survey setup now?";
}


const uiString uiSEGYSurvInfoProvider::importAskUiQuestion() const
{
    return uiStrings::phrImport(
			      tr("the SEG-Y data used for survey setup now?"));
}
