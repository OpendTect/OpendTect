/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh
 * DATE     : June 2007
-*/


#include "uitutwelltools.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uispinbox.h"
#include "uilabel.h"
#include "uimsg.h"
#include "bufstring.h"
#include "ioobj.h"
#include "od_ostream.h"
#include "wellmanager.h"
#include "wellodio.h"
#include "welllogset.h"
#include "tutlogtools.h"

static const StepInterval<int> sSampleGateRange( 3, 99, 2 );

uiTutWellTools::uiTutWellTools( uiParent* p, const DBKey& wellid )
	: uiDialog( p, Setup( tr("Log Smoothing"),
			      tr("Specify parameters for smoothing"),
			      HelpKey("tut","well") ) )
{
    uiRetVal uirv;
    wd_ = Well::MGR().fetchForEdit( wellid, Well::LoadReqs(Well::Logs), uirv );
    if ( !wd_ )
	{ new uiLabel( this, uirv ); return; }

    const Well::LogSet& logs = wd_->logs();
    uiListBox::Setup su( OD::ChooseOnlyOne, tr("Select Input Log") );
    inplogfld_ = new uiListBox( this, su );
    inplogfld_->setHSzPol( uiObject::Wide );
    BufferStringSet nms; logs.getNames( nms );
    inplogfld_->addItems( nms );
    inplogfld_->selectionChanged.notify(
				mCB(this,uiTutWellTools,inpchg) );

    outplogfld_ = new uiGenInput( this, tr("Specify Output Log name"),
				StringInpSpec( "" ) );
    outplogfld_->setElemSzPol( uiObject::Wide );
    outplogfld_->attach( alignedBelow, inplogfld_ );

    gatefld_ = new uiLabeledSpinBox( this, tr("Sample Gate") );
    gatefld_->box()->setInterval( sSampleGateRange );
    gatefld_->attach( alignedBelow, outplogfld_ );
}


void uiTutWellTools::inpchg( CallBacker* )
{
    const BufferString newlognm( inplogfld_->getText(),
				 " [smoothed by uiTutWellTools]" );
    outplogfld_->setText( newlognm );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiTutWellTools::acceptOK()
{
    if ( !wd_ )
	return false;

    const char* inplognm = inplogfld_->getText();
    Well::LogSet& logset = wd_->logs();
    const int inpidx = logset.indexOf( inplognm );
    if ( inpidx<0 || inpidx>=logset.size() )
	mErrRet( uiStrings::phrSelect(tr("a valid Input Log")) )

    const char* lognm = outplogfld_->text();
    const int outpidx = logset.indexOf( lognm );
    if ( outpidx>=0 && outpidx<logset.size() )
	mErrRet( tr("Output Log already exists\n Enter a new name") )

    if ( !lognm || !*lognm )
	mErrRet( uiStrings::phrEnter(tr("a valid name for Output log")) )

    const int gate = gatefld_->box()->getIntValue();
    RefMan<Well::Log> outputlog = new Well::Log( lognm );
    Tut::LogTools logtool( *logset.getLogByIdx(inpidx), *outputlog );
    if ( logtool.runSmooth(gate) )
    {
	logset.add( outputlog );
	SilentTaskRunnerProvider trprov;
	uiRetVal uirv = Well::MGR().save( *wd_, trprov );
	if ( uirv.isError() )
	    mErrRet( uirv )
    }

    return true;
}
