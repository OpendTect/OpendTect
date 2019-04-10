/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Oct 2010
________________________________________________________________________

-*/

#include "uiseisimpcubefromothersurv.h"

#include "ctxtioobj.h"
#include "seisselsetup.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uimsg.h"
#include "uiseissubsel.h"
#include "uiseissel.h"
#include "uisurvioobjseldlg.h"
#include "uiseparator.h"
#include "uitaskrunner.h"
#include "od_helpids.h"


static const char* interpols[] = { "Sinc interpolation", "Nearest trace", 0 };

uiSeisImpCubeFromOtherSurveyDlg::uiSeisImpCubeFromOtherSurveyDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Import cube from other survey"),
		       tr("Specify import parameters"),
		       mODHelpKey(mSeisImpCBVSFromOtherSurveyDlgHelpID))
		 .modal(false))
    , import_(0)
{
    setCtrlStyle( RunAndClose );

    finpfld_ = new uiGenInput( this, uiStrings::sFileName(), FileNameInpSpec());
    finpfld_->setReadOnly();
    CallBack cb = mCB(this,uiSeisImpCubeFromOtherSurveyDlg,cubeSel);
    uiButton* selbut = uiButton::getStd( this, OD::Select, cb, true );
    selbut->attach( rightOf, finpfld_ );

    subselfld_ = new uiSeis3DSubSel( this, Seis::SelSetup(false) );
    subselfld_->attach( alignedBelow, finpfld_ );

    uiSeparator* sep1 = new uiSeparator( this, "sep" );
    sep1->attach( stretchedBelow, subselfld_ );

    interpfld_ = new uiGenInput( this, uiStrings::sInterpolation(),
			    BoolInpSpec( true, toUiString(interpols[0]),
			    toUiString(interpols[1]) ) );
    interpfld_->valuechanged.notify(
		mCB(this,uiSeisImpCubeFromOtherSurveyDlg,interpSelDone) );
    interpfld_->attach( ensureBelow, sep1 );
    interpfld_->attach( alignedBelow, subselfld_ );

    cellsizefld_ = new uiLabeledSpinBox(this, tr("Lateral stepout (Inl/Crl)"));
    cellsizefld_->attach( alignedBelow, interpfld_ );
    cellsizefld_->box()->setInterval( 2, 12, 2 );
    cellsizefld_->box()->setValue( 8 );

    uiSeparator* sep2 = new uiSeparator( this, "sep" );
    sep2->attach( stretchedBelow, cellsizefld_ );

    outfld_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Vol,false),
			     uiSeisSel::Setup(Seis::Vol) );
    outfld_->attach( alignedBelow, cellsizefld_ );
    outfld_->attach( ensureBelow, sep2 );

    interpSelDone(0);
}


uiSeisImpCubeFromOtherSurveyDlg::~uiSeisImpCubeFromOtherSurveyDlg()
{
    delete import_;
}


void uiSeisImpCubeFromOtherSurveyDlg::interpSelDone( CallBacker* )
{
    const bool curitm = interpfld_->getBoolValue() ? 0 : 1;
    interpol_ = (SeisCubeImpFromOtherSurvey::Interpol)(curitm);
    issinc_ = interpol_ == SeisCubeImpFromOtherSurvey::Sinc;
    cellsizefld_->display( issinc_ );
}


void uiSeisImpCubeFromOtherSurveyDlg::cubeSel( CallBacker* )
{
    uiSurvIOObjSelDlg objsel( this, uiSeisSel::ioContext( Seis::Vol, true ) );
    objsel.excludeCurrentSurvey();
    if ( objsel.go() )
    {
	if ( import_ )
	    delete import_;
	import_ = new SeisCubeImpFromOtherSurvey( *objsel.ioObj() );
	BufferString mainfnm = objsel.mainFileName();
	if ( import_->prepareRead( mainfnm ) )
	{
	    finpfld_->setText( mainfnm );
	    subselfld_->setInput( import_->cubeSampling() );
	}
	else
	    { delete import_; import_ = 0; }
    }
}


#define mErrRet(msg ) { uiMSG().error( msg ); return false; }
bool uiSeisImpCubeFromOtherSurveyDlg::acceptOK()
{
    if ( !import_ )
	mErrRet( tr("No valid input, please select a new input file") )

    const IOObj* outioobj = outfld_->ioobj();
    if ( !outioobj )
	return false;

    const int cellsz = issinc_ ? cellsizefld_->box()->getIntValue() : 0;
    TrcKeyZSampling cs; subselfld_->getSampling( cs );
    import_->setPars( interpol_, cellsz, cs );
    import_->setOutput( const_cast<IOObj&>(*outioobj) );
    uiTaskRunner taskrunner( this );

    if ( !TaskRunner::execute( &taskrunner, *import_ ) )
	return false;

    uiString msg = tr("Cube successfully imported\n\n"
		      "Do you want to import more cubes?");
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				 tr("No, close window") );
    return !ret;
}
