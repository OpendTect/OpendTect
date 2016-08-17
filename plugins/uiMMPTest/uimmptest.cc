/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		August 2016
________________________________________________________________________

-*/

#include "uimmptest.h"
#include "uilabel.h"

#include "batchjobdispatch.h"
#include "hostdata.h"
#include "iopar.h"
#include "jobrunner.h"
#include "od_ostream.h"
#include "od_helpids.h"

static const char* sKeyMMPTestProgName = "od_mmptestbatch.cc";


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
    setCancelText( uiString::emptyString() );
    setCaption( tr("Multi-Machine Processing Diagnostic Tool") );
}


uiMMPTestProc::~uiMMPTestProc()
{
}


bool uiMMPTestProc::initWork( bool retry )
{
    errmsg_.setEmpty();
    BufferString tmpstordir;

    delete jobrunner_;
    IOPar dummy;
    jobrunner_ = new JobRunner( new MMTestJobDescProv(dummy),
				sKeyMMPTestProgName );
    return true;
}



MMTestJobDescProv::MMTestJobDescProv( const IOPar& iop )
    : JobDescProv(iop)
    , hdl_(*new HostDataList(false))
{
}


MMTestJobDescProv::~MMTestJobDescProv()
{
    delete &hdl_;
}


int MMTestJobDescProv::nrJobs() const
{
    return hdl_.size();
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
    mDeclStaticString( namestr );
    namestr = toString( jidx + 1 );
    return namestr.buf();
}

