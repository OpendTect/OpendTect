/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackmmproc.h"
#include "uiprestackmmjobdispatch.h"
#include "jobrunner.h"
#include "jobdescprov.h"
#include "iopar.h"
#include "file.h"
#include "filepath.h"
#include "seistype.h"
#include "genc.h"
#include "prestackprocessor.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "posinfo.h"

#include "uimsg.h"


bool Batch::PreStackMMProgDef::isSuitedFor( const char* pnm ) const
{
    StringView prognm = pnm;
    return prognm == Batch::JobSpec::progNameFor( Batch::JobSpec::PreStack );
}

bool Batch::PreStackMMProgDef::canHandle( const Batch::JobSpec& js ) const
{
    return isSuitedFor( js.prognm_ );
}


uiPreStackMMProc::uiPreStackMMProc( uiParent* p, const IOPar& iop )
    : uiMMBatchJobDispatcher(p,iop,mODHelpKey(mPreStackMMProcHelpID))
    , is2d_(Seis::is2DGeom(iop))
{
    setTitleText( tr("Distributed Computing - Prestack") );
}


uiPreStackMMProc::~uiPreStackMMProc()
{
}


#define mErrRet(s) { uiMSG().error(s); return 0; }

bool uiPreStackMMProc::initWork( bool retry )
{
    if ( is2d_ )
    {
	//TODO remove when 2D prestack processing implemented
	mErrRet(tr("No 2D Prestack Processing available") )
    }

    MultiID inpmid;
    jobpars_.get( PreStack::ProcessManager::sKeyInputData(), inpmid );
    if ( inpmid.isUdf() )
	mErrRet(tr("Key for input data store missing in job specification") )

    PtrMan<IOObj> ioobj = IOM().get( inpmid );
    if ( !ioobj )
	mErrRet(tr("Cannot find data store with ID: %1").
							arg(inpmid.toString()))

    PtrMan<SeisPS3DReader> rdr = SPSIOPF().get3DReader( *ioobj );
    if ( !rdr )
	mErrRet( uiStrings::phrCannotOpen( ioobj->uiName() ) );

    const PosInfo::CubeData& cd = rdr->posData();
    TypeSet<int> inlnrs; int previnl = mUdf(int);
    PosInfo::CubeDataPos cdp;
    TrcKeySampling jobhs; jobhs.usePar( jobpars_ );
    while ( cd.toNext(cdp) )
    {
	const BinID curbid = cd.binID( cdp );
	if ( curbid.inl() != previnl && jobhs.includes(curbid) )
	    { inlnrs += curbid.inl(); previnl = curbid.inl(); }
    }

    if ( inlnrs.isEmpty() )
	mErrRet(tr("Selected area not present in data store"))

    InlineSplitJobDescProv* dp = new InlineSplitJobDescProv( jobpars_, inlnrs );
    dp->setNrInlsPerJob( 1 );

    delete jobrunner_;
    jobrunner_ = new JobRunner( dp, "od_process_prestack" );
    return true;
}
