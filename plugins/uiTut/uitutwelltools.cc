/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh
 * DATE     : June 2007
-*/


#include "uitutwelltools.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uispinbox.h"
#include "uimsg.h"
#include "bufstring.h"
#include "ioobj.h"
#include "ioman.h"
#include "od_ostream.h"
#include "welldata.h"
#include "wellio.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "wellwriter.h"
#include "tutlogtools.h"
#include "od_helpids.h"


static const StepInterval<int> sSampleGateRange( 3, 99, 2 );

uiTutWellTools::uiTutWellTools( uiParent* p, const MultiID& wellid )
	: uiDialog( p, Setup( tr("Log Smoothing"),
			      tr("Specify parameters for smoothing"),
			      mODHelpKey(mStorePicksHelpID) ) )
	, wellid_(wellid)
	, wd_(Well::MGR().get(wellid))
{
    if ( !wd_ )
	return;

    wd_->ref();
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


uiTutWellTools::~uiTutWellTools()
{
    if ( wd_ )
	wd_->unRef();
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
    if ( !wd_ ) return false;
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
	Well::LogSet::LogID logid = logset.add( outputlog );
	PtrMan<IOObj> ioobj = IOM().get( wellid_ );
	if ( !ioobj )
	    mErrRet( uiStrings::phrCannotFind(tr("object in I/O Manager")) )

	Well::Writer wtr( *ioobj, *wd_ );
	const Well::Log* newlog = logset.getLog( logid );
	if ( newlog && !wtr.putLog(*newlog) )
	{
	    uiString errmsg = uiStrings::phrCannotWrite(tr("log: %1"
			     "\n Check write permissions.").arg(newlog->name()));
	    mErrRet( errmsg )
	}
    }

    return true;
}
