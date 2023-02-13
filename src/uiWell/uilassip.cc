/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uilassip.h"

#include "uibulkwellimp.h"
#include "uicoordsystem.h"
#include "uidialog.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "uisurvinfoed.h"

#include "ctxtioobj.h"
#include "file.h"
#include "od_helpids.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "wellimpasc.h"


// uiLASSurvInfoProvider
class uiLASReadDlg : public uiDialog
{ mODTextTranslationClass(uiLASReadDlg)
public:
uiLASReadDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("LAS well data selection"),
				 mNoDlgTitle,mNoHelpKey ).modal(true))
{
    setOkCancelText( uiStrings::sContinue(), uiStrings::sCancel() );

    inpfld_ = new uiFileInput( this, uiStrings::phrInput(tr("LAS files")),
						      uiFileInput::Setup() );
    inpfld_->setSelectMode( uiFileDialog::ExistingFiles );
    mAttachCB( inpfld_->valuechanged, uiLASReadDlg::lasSel );

    crssel_ = new Coords::uiCoordSystemSel( this, true, false );
    crssel_->attach( alignedBelow, inpfld_ );
}


~uiLASReadDlg()
{
    detachAllNotifiers();
}


void lasSel( CallBacker* )
{
    BufferStringSet filenms;
    inpfld_->getFileNames( filenms );
    coords_.setEmpty();

    for ( int idx=0; idx<filenms.size(); idx++ )
    {
	const BufferString& fnm = filenms.get( idx );
	if ( !File::exists(fnm) || File::isEmpty(fnm) )
	{ uiMSG().error(uiStrings::sInvInpFile()); continue; }

	Well::LASImporter lasimp;
	Well::LASImporter::FileInfo info;
	BufferString errmsg = lasimp.getLogInfo( fnm, info );
	if ( errmsg.isEmpty() )
	{
	    if ( !info.loc_.isUdf() )
		coords_ += info.loc_;
	    zrg_.include( info.zrg_ );
	}
	else
	{
	    uiMSG().error( toUiString( errmsg ) );
	}
    }
}


bool acceptOK( CallBacker* ) override
{
    return true;
}

    uiFileInput*		inpfld_;

    Coords::uiCoordSystemSel*	crssel_;
    TypeSet<Coord>		coords_;
    Interval<float>		zrg_;

};

uiLASSurvInfoProvider::uiLASSurvInfoProvider()
{
}


uiLASSurvInfoProvider::~uiLASSurvInfoProvider()
{
}


const char* uiLASSurvInfoProvider::usrText() const
{ return "Scan LAS Well Data"; }

const char* uiLASSurvInfoProvider::iconName() const
{ return "welllog.small"; }


uiDialog* uiLASSurvInfoProvider::dialog( uiParent* p )
{
    return new uiLASReadDlg( p );
}


bool uiLASSurvInfoProvider::getInfo( uiDialog* dlg, TrcKeyZSampling& tkzs,
				     Coord crd[3] )
{
    const double grdsp = 25.;
    const float zstep = 10.0;

    mDynamicCastGet(uiLASReadDlg*,lasdlg,dlg)
    if ( !lasdlg )
	return false;

    coordsystem_ = lasdlg->crssel_->getCoordSystem();
    lasdlg->inpfld_->getFileNames( filenms_ );

    Interval<double> xrg( mUdf(double), -mUdf(double) );
    Interval<double> yrg( mUdf(double), -mUdf(double) );
    const TypeSet<Coord>& coords = lasdlg->coords_;
    if ( coords.isEmpty() )
    {
	xrg.include( 0.0, false );
	yrg.include( 0.0, false );
	xrg.widen( 10*grdsp, false );
	yrg.widen( 10*grdsp, false );
    }
    else if ( coords.size()==1 )
    {
	xrg.include( coords[0].x, false );
	xrg.widen( 10*grdsp, false );
	yrg.include( coords[0].y, false );
	yrg.widen( 10*grdsp, false );
    }
    else
    {
	for ( int idx=0; idx<coords.size(); idx++ )
	{
	    xrg.include( coords[idx].x, false );
	    yrg.include( coords[idx].y, false );
	}
    }

    Coord c0( xrg.start, yrg.start );
    Coord c1( xrg.stop, yrg.stop );
    const Interval<float> zrg = lasdlg->zrg_;
    tkzs.zsamp_.set( zrg.start, zrg.stop, zstep );
    return uiSurvInfoProvider::getRanges(tkzs,crd,c0,c1,grdsp);
}


void uiLASSurvInfoProvider::fillLogPars( IOPar& par ) const
{
    uiSurvInfoProvider::fillLogPars( par );
}


IOPar* uiLASSurvInfoProvider::getImportPars() const
{
    IOPar* imppars = new IOPar;
    imppars->set(sKey::ID(),
		 IOObjContext::getStdDirData(IOObjContext::WllInf)->id_ );
    return imppars;
}


void uiLASSurvInfoProvider::startImport( uiParent* p, const IOPar& pars )
{
    uiBulkLogImport dlg( p, filenms_ );
    dlg.setModal( true );
    dlg.go();
}


const char* uiLASSurvInfoProvider::importAskQuestion() const
{ return "Proceed to import files used to setup survey?"; }


IOPar* uiLASSurvInfoProvider::getCoordSystemPars() const
{
    if ( !coordsystem_ )
	return nullptr;

    auto* crspar = new IOPar;
    coordsystem_->fillPar( *crspar );
    return crspar;
}
