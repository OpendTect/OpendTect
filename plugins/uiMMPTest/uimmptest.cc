/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		August 2016
________________________________________________________________________

-*/

#include "uimmptest.h"
#include "uimsg.h"

#include "batchjobdispatch.h"
#include "filepath.h"
#include "hostdata.h"
#include "iopar.h"
#include "jobrunner.h"
#include "od_ostream.h"
#include "od_helpids.h"
#include "keystrs.h"

static const char* sKeyMMPTestProgName = "od_mmptestbatch";


bool Batch::TestMMProgDef::isSuitedFor( const char* pnm ) const
{
    FixedString prognm = pnm;
    return prognm == sKeyMMPTestProgName;
}

bool Batch::TestMMProgDef::canHandle( const Batch::JobSpec& js ) const
{
    return isSuitedFor( js.prognm_ );
}

bool Batch::TestMMProgDef::canResume( const Batch::JobSpec& js ) const
{
    return false;
}


uiMMPTestProc::uiMMPTestProc( uiParent* p, const IOPar& iop )
    : uiMMBatchJobDispatcher(p,iop,mNoHelpKey)
{
    setOkText( uiStrings::sClose() );
    setCancelText( uiString::empty() );
    setCaption( tr("Distributed Processing Diagnostic Tool") );
    enableJobControl( false );

    File::Path fp( iop.find(sKey::FileName()) );
    fp.setExtension( ".log" );
    logstrm_ = new od_ostream( fp.fullPath() );

    setTitleText( tr("Select Machines from the list on left and press 'Add'"
		" to run Diagnostic Test on them") );
}


uiMMPTestProc::~uiMMPTestProc()
{
}


bool uiMMPTestProc::initWork( bool retry )
{
    errmsg_.setEmpty();
    BufferString tmpstordir;

    delete jobrunner_;
    jobrunner_ = new JobRunner( new MMTestJobDescProv(jobpars_,nrSelMachs()),
				sKeyMMPTestProgName, logstrm_ );
    return true;
}


bool uiMMPTestProc::acceptOK()
{
    const BufferString filenm = logstrm_->fileName();
    delete logstrm_; logstrm_ = 0;
    uiMSG().message( tr("The log file has been saved as %1").arg(filenm) );
    return true;
}


bool uiMMPTestProc::rejectOK()
{
    const BufferString filenm = logstrm_->fileName();
    delete logstrm_; logstrm_ = 0;
    uiMSG().message( tr("The log file has been saved as %1").arg(filenm) );
    return true;
}


bool uiMMPTestProc::prepareCurrentJob()
{
    const JobInfo& ji = jobrunner_->curJobInfo();
    BufferString msg( "Attempting to run " );
    msg += jobrunner_->descProv()->objType();
    if ( ji.hostdata_ )
    {
	msg += " on ";
	msg += ji.hostdata_->getHostName();
    }

    *logstrm_ << msg;
    return true;
}


MMTestJobDescProv::MMTestJobDescProv( const IOPar& iop, int nrmachs )
    : JobDescProv(iop)
    , nrmachs_(nrmachs)
{
}


MMTestJobDescProv::~MMTestJobDescProv()
{
}


int MMTestJobDescProv::nrJobs() const
{
    return nrmachs_;
}


const char* MMTestJobDescProv::objType() const
{
    return "Diagnostic Test";
}


void MMTestJobDescProv::getJob( int jidx, IOPar& jobpar ) const
{
    jobpar = inpiopar_;
}


void MMTestJobDescProv::dump( od_ostream& strm ) const
{
    strm << "\nMMP Test JobDescProv dump.\n"
	    "The following jobs description keys are available:\n";
    for ( int idx=0; idx<nrJobs(); idx++ )
	strm << objName( idx ) << "; ";
    strm << od_endl;
}


const char* MMTestJobDescProv::objName( int jidx ) const
{
    return "";
}

