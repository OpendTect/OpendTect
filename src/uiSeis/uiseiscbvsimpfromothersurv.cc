/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseiscbvsimpfromothersurv.h"

#include "ctxtioobj.h"
#include "hiddenparam.h"
#include "ioman.h"
#include "surveydisklocation.h"
#include "seiscbvsimpfromothersurv.h"
#include "zdomain.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uimsg.h"
#include "uiseissubsel.h"
#include "uiseissel.h"
#include "uiselobjothersurv.h"
#include "uiseparator.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

static HiddenParam<uiSeisImpCBVSFromOtherSurveyDlg,SurveyDiskLocation*>
							hp_sdl( nullptr );

static const char* interpols[] = { "Sinc interpolation", "Nearest trace", 0 };

uiSeisImpCBVSFromOtherSurveyDlg::uiSeisImpCBVSFromOtherSurveyDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Import CBVS cube from other survey"),
		       tr("Specify import parameters"),
		       mODHelpKey(mSeisImpCBVSFromOtherSurveyDlgHelpID))
		 .modal(false))
    , import_(nullptr)
{
    hp_sdl.setParam( this, nullptr );
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    finpfld_ = new uiGenInput( this, tr("CBVS file name") );
    finpfld_->setElemSzPol( uiObject::Wide );
    finpfld_->setReadOnly();
    CallBack cb = mCB(this,uiSeisImpCBVSFromOtherSurveyDlg,cubeSel);
    uiPushButton* selbut = new uiPushButton( this, m3Dots(uiStrings::sSelect()),
					     cb, true );
    selbut->attach( rightOf, finpfld_ );

    subselfld_ = new uiSeis3DSubSel( this, Seis::SelSetup( false ) );
    subselfld_->attach( alignedBelow, finpfld_ );

    uiSeparator* sep1 = new uiSeparator( this, "sep" );
    sep1->attach( stretchedBelow, subselfld_ );

    interpfld_ = new uiGenInput( this, tr("Interpolation"),
			BoolInpSpec( true, toUiString(interpols[0]),
				     toUiString(interpols[1]) ) );
    interpfld_->valueChanged.notify(
		mCB(this,uiSeisImpCBVSFromOtherSurveyDlg,interpSelDone) );
    interpfld_->attach( ensureBelow, sep1 );
    interpfld_->attach( alignedBelow, subselfld_ );
    interpfld_->setValue ( false );
    interpfld_->setSensitive( false );

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

    interpSelDone(nullptr);
}


uiSeisImpCBVSFromOtherSurveyDlg::~uiSeisImpCBVSFromOtherSurveyDlg()
{
    hp_sdl.removeAndDeleteParam( this );
}


void uiSeisImpCBVSFromOtherSurveyDlg::interpSelDone( CallBacker* )
{
    const bool curitm = interpfld_->getBoolValue() ? 0 : 1;
    interpol_ = (SeisImpCBVSFromOtherSurvey::Interpol)(curitm);
    issinc_ = interpol_ == SeisImpCBVSFromOtherSurvey::Sinc;
    cellsizefld_->display( issinc_ );
}


void uiSeisImpCBVSFromOtherSurveyDlg::cubeSel( CallBacker* )
{
    CtxtIOObj inctio( uiSeisSel::ioContext( Seis::Vol, true ) );
    uiSelObjFromOtherSurvey objdlg( this, inctio );
    SurveyDiskLocation* sdl = hp_sdl.getParam( this );
    if ( sdl )
	objdlg.setDirToOtherSurvey( *sdl );

    if ( objdlg.go() && inctio.ioobj_ )
    {
	if ( sdl )
	    *sdl = objdlg.getSurveyDiskLocation();
	else
	    hp_sdl.setParam( this,
		    new SurveyDiskLocation(objdlg.getSurveyDiskLocation()) );

	delete import_;
	import_ = new SeisImpCBVSFromOtherSurvey( *inctio.ioobj_ );
	BufferString fusrexp; objdlg.getIOObjFullUserExpression( fusrexp );
	bool needinterpol = false;
	if ( import_->prepareRead(fusrexp) )
	{
	    finpfld_->setText( fusrexp );
	    subselfld_->setInput( import_->cubeSampling() );
	    needinterpol = !import_->hasSameGridAsThisSurvey();
	}
	else
	{
	    uiMSG().error( import_->errMsg() );
	    deleteAndNullPtr( import_ );
	}

	interpfld_->setValue ( needinterpol );
	interpfld_->setSensitive( needinterpol );
	interpSelDone(nullptr);

	outfld_->setInputText( inctio.ioobj_->name() );
    }
}


#define mErrRet(msg ) { uiMSG().error( msg ); return false; }
bool uiSeisImpCBVSFromOtherSurveyDlg::acceptOK( CallBacker* )
{
    if ( !import_ )
	mErrRet( tr("No valid input, please select a new input file") )

    const IOObj* outioobj = outfld_->ioobj();
    if ( !outioobj )
	return false;

    if ( outfld_->getZDomain().fillPar(outioobj->pars()) &&
	 !IOM().commitChanges(*outioobj) )
    {
	uiMSG().error( uiStrings::phrCannotWriteDBEntry(outioobj->uiName()) );
	return false;
    }

    int cellsz = issinc_ ? cellsizefld_->box()->getIntValue() : 0;
    TrcKeyZSampling cs; subselfld_->getSampling( cs );
    import_->setPars( interpol_, cellsz, cs );
    import_->setOutput( const_cast<IOObj&>(*outioobj) );
    uiTaskRunner taskrunner( this );

    if ( !TaskRunner::execute(&taskrunner,*import_) )
	return false;

    const uiString msg = tr("CBVS cube successfully imported\n\n"
			    "Do you want to import more cubes?");
    const bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				 tr("No, close window") );
    return !ret;
}
