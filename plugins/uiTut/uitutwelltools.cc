/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh
 * DATE     : June 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

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
    const Well::LogSet& logs = wd_->logs(); 
    inplogfld_ = new uiLabeledListBox( this, tr("Select Input Log") );
    inplogfld_->box()->setHSzPol( uiObject::Wide );
    for ( int idx=0; idx<logs.size(); idx++ )
	inplogfld_->box()->addItem( logs.getLog(idx).name() );
    inplogfld_->box()->selectionChanged.notify( 
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
}


void uiTutWellTools::inpchg( CallBacker* )
{
    BufferString lognm = inplogfld_->box()->getText();
    lognm += "_Smooth";
    outplogfld_->setText( lognm );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiTutWellTools::acceptOK( CallBacker* )
{
    const char* inplognm = inplogfld_->box()->getText();
    Well::LogSet& logset = wd_->logs();
    const int inpidx = logset.indexOf( inplognm );
    if ( inpidx<0 || inpidx>=logset.size() )
	mErrRet( tr("Please select a valid Input Log") )

    const char* lognm = outplogfld_->text();
    const int outpidx = logset.indexOf( lognm );
    if ( outpidx>=0 && outpidx<logset.size() )
	mErrRet( tr("Output Log already exists\n Enter a new name") )

    if ( !lognm || !*lognm )
	mErrRet( tr("Please enter a valid name for Output log") )

    const int gate = gatefld_->box()->getValue();
    Well::Log* outputlog = new Well::Log( lognm );
    Tut::LogTools logtool( logset.getLog(inpidx), *outputlog );
    if ( logtool.runSmooth(gate) )
    {
	logset.add( outputlog );
	PtrMan<IOObj> ioobj = IOM().get( wellid_ );
	if ( !ioobj )
	    mErrRet( tr("Cannot find object in I/O Manager") )

	Well::Writer wtr( ioobj->fullUserExpr(), *wd_ );
	const int lognr = logset.size();
	const BufferString logfnm =
	    		wtr.getFileName( Well::IO::sExtLog(), lognr );
	od_ostream strm( logfnm );
	if ( !strm.isOK() )
	{
	    BufferString errmsg( "Cannot open file for write" );
	    strm.addErrMsgTo( errmsg );
	    mErrRet( errmsg )
	}
	wtr.putLog( strm, logset.getLog(lognr-1) );
    }
    else
	delete outputlog;

    return true;
}
