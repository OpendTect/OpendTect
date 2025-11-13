/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitutwelltools.h"


#include "bufstring.h"
#include "ioobj.h"
#include "ioman.h"
#include "od_ostream.h"
#include "tutlogtools.h"
#include "welldata.h"
#include "wellio.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "wellwriter.h"

#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uispinbox.h"

static const StepInterval<int> sSampleGateRange( 3, 99, 2 );

uiTutWellTools::uiTutWellTools( uiParent* p, const MultiID& wellid )
    : uiDialog( p, Setup( tr("Log Smoothing"),
			  tr("Specify parameters for smoothing"),
			  HelpKey("tut","well") ) )
    , wd_(Well::MGR().get(wellid, Well::LoadReqs(Well::LogInfos)))
    , wellid_(wellid)
{
    if ( !wd_ )
	return;

    const Well::LogSet& logs = wd_->logs();
    uiListBox::Setup su( OD::ChooseOnlyOne, tr("Select Input Log") );
    inplogfld_ = new uiListBox( this, su, "lognames" );
    inplogfld_->setHSzPol( uiObject::Wide );
    BufferStringSet lognms;
    logs.getNames( lognms );
    inplogfld_->addItems( lognms );

    mAttachCB( inplogfld_->selectionChanged, uiTutWellTools::inpchg );

    outplogfld_ = new uiGenInput( this, tr("Specify Output Log name"),
				StringInpSpec( "" ) );
    outplogfld_->setElemSzPol( uiObject::Wide );
    outplogfld_->attach( alignedBelow, inplogfld_ );

    gatefld_ = new uiLabeledSpinBox( this, tr("Sample Gate") );
    gatefld_->box()->setInterval( sSampleGateRange );
    gatefld_->attach( alignedBelow, outplogfld_ );
}


uiTutWellTools::~uiTutWellTools()
{
    detachAllNotifiers();
}


void uiTutWellTools::inpchg( CallBacker* )
{
    BufferString lognm = inplogfld_->getText();
    lognm += "_Smooth";
    outplogfld_->setText( lognm );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiTutWellTools::acceptOK( CallBacker* )
{
    if ( !wd_ )
	return false;

    const BufferString inplognm = inplogfld_->getText();
    Well::LogSet& logs = wd_->logs();
    if ( !logs.isPresent(inplognm.buf()) )
	mErrRet( tr("Please select a valid Input Log") )

    const BufferString lognm = outplogfld_->text();
    if ( lognm.isEmpty() )
	mErrRet( tr("Please enter a valid name for Output log") )

    if ( logs.isPresent(lognm.str()) )
	mErrRet( tr("Output Log already exists. Enter a new name") )

    Well::LoadReqs lreqs( Well::LogInfos );
    lreqs.addLog( inplognm.buf() );
    wd_ = Well::MGR().get( wd_->multiID(), lreqs );
    if ( !wd_ )
	mErrRet( Well::MGR().errMsg() );

    const Well::Log* inplog = logs.getLog( inplognm.buf() );
    PtrMan<Well::Log> outputlog = new Well::Log( lognm );
    Tut::LogTools logtool( *inplog, *outputlog );
    const int gate = gatefld_->box()->getIntValue();
    if ( logtool.runSmooth(gate) )
    {
	logs.add( outputlog.release() );
	PtrMan<IOObj> ioobj = IOM().get( wellid_ );
	if ( !ioobj )
	    mErrRet( tr("Cannot find object in I/O Manager") )

	Well::Writer wtr( *ioobj, *wd_ );
	const Well::Log& newlog = logs.last();
	if ( !wtr.putLog(newlog) )
	{
	    uiString errmsg = tr("Could not write log: %1"
				 "\n Check write permissions.")
			    .arg(newlog.name());
	    mErrRet( errmsg )
	}
    }
    else
    {
	uiMSG().error( tr("Smoothing operation failed") );
	return false;
    }

    const bool ret = uiMSG().askGoOn(
	    tr("Process finished successfully. Do you want to continue?") );
    return !ret;
}
