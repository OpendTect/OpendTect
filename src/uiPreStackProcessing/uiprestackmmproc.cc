/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          April 2002 / Mar 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiprestackmmproc.h"
#include "uiprestackmmjobdispatch.h"
#include "jobrunner.h"
#include "iopar.h"
#include "file.h"
#include "filepath.h"
#include "seistype.h"
#include "settings.h"
#include "genc.h"

#include "uilabel.h"


bool Batch::PreStackMMProgDef::isSuitedFor( const char* pnm ) const
{
    FixedString prognm = pnm;
    return prognm == Batch::JobSpec::progNameFor( Batch::JobSpec::PreStack );
}

bool Batch::PreStackMMProgDef::canHandle( const Batch::JobSpec& js ) const
{
    return isSuitedFor( js.prognm_ );
}



uiPreStackMMProc::uiPreStackMMProc( uiParent* p, const IOPar& iop )
    : uiMMBatchJobDispatcher(p,iop,mTODOHelpKey)
    , is2d_(Seis::is2DGeom(iop))
{
    setTitleText( isMultiHost()  ? "Multi-Machine PreStack Processing"
				 : "Line-split PreStack processing" );
}


uiPreStackMMProc::~uiPreStackMMProc()
{
}


#define mErrRet(s) { uiMSG().error(s); return 0; }

bool uiPreStackMMProc::initWork( bool retry )
{
    delete jobrunner_;
    jobrunner_ = 0;
    errmsg_.set( "TODO: implement uiPreStackMMProc::initWork" );
    return false;
}


bool uiPreStackMMProc::prepareCurrentJob()
{
    return true;
}


Executor* uiPreStackMMProc::getPostProcessor() const
{
    return 0;
}


bool uiPreStackMMProc::needConfirmEarlyStop() const
{
    return jobrunner_;
}
