/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2013
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "matlablibmgr.h"

#include "file.h"
#include "filepath.h"
#include "ptrman.h"
#include "sharedlibs.h"

#include "matlabarray.h"

#ifdef HAS_MATLAB
extern "C" {
    void mclmcrInitialize();
    bool mclInitializeApplication(const char**,size_t);
    void mclTerminateApplication();

    typedef bool (*initfn)();		// XX_cInitialize(void)
    typedef void (*termfn)();		// XX_cTerminate(void)

    // mlfOd_getparameters
    typedef bool (*getparfn)(int,mxArray**,mxArray**);
};
#endif

// MatlabLibAccess
MatlabLibAccess::MatlabLibAccess( const char* libfnm )
    : shlibfnm_(libfnm)
    , inited_(false)
{
}


MatlabLibAccess::~MatlabLibAccess()
{
    if ( inited_ ) terminate();
}


#ifdef HAS_MATLAB
static BufferString getFnm( const char* libfnm, bool init )
{
    FilePath fp( libfnm );
    fp.setExtension( "" );
    return BufferString ( fp.fileName(), init ? "Initialize" : "Terminate" );
}


#define mErrRet(msg) { errmsg_ = msg; return false; }

#define mGetSLA() \
    const SharedLibAccess* sla = MLM().getSharedLibAccess( shlibfnm_ ); \
    if ( !sla ) mErrRet( "Cannot access shared library" );

bool MatlabLibAccess::init()
{
    if ( inited_ ) return true;

    if ( !File::exists(shlibfnm_) )
	mErrRet( BufferString("Cannot find shared library ",shlibfnm_) );

    if ( !MLM().isLoaded(shlibfnm_) && !MLM().load(shlibfnm_) )
	mErrRet( BufferString("Cannot load shared library ",shlibfnm_) );

    mGetSLA();

    const BufferString initfnm = getFnm( shlibfnm_, true );
    initfn ifn = (initfn)sla->getFunction( initfnm.buf() );
    if ( !ifn )
	mErrRet( BufferString("Cannot find function ",initfnm) );

    if ( !(*ifn)() )
	mErrRet( "Cannot initialize shared library" );

    inited_ = true;
    return true;
}


bool MatlabLibAccess::terminate()
{
    if ( !inited_ ) return true;

    mGetSLA();
    const BufferString termfnm = getFnm( shlibfnm_, false );
    termfn tfn = (termfn)sla->getFunction( termfnm.buf() );
    if ( !tfn )
	mErrRet( BufferString("Cannot find function ",termfnm) );
    (*tfn)();

    inited_ = false;
    return true;
}


bool MatlabLibAccess::getParameters( BufferStringSet& names,
				     BufferStringSet& values ) const
{
    mGetSLA();

    const char* getparfnm = "mlfOd_getparameters";
    getparfn fn = (getparfn)sla->getFunction( getparfnm );
    if ( !fn )
	mErrRet( BufferString("Cannot find function ",getparfnm) );

    mxArray* nrparsarr = NULL;
    mxArray* varargs = NULL;
    bool res = (*fn)( 1, &nrparsarr, &varargs );
    const double nrpars = mxGetScalar( nrparsarr );

    const int nrargs = (int)nrpars*2 + 1;
    res = (*fn)( nrargs, &nrparsarr, &varargs );
    if ( !res )
       mErrRet( BufferString("Function ",getparfnm," returned false") );

    mxArray* pars = mxGetCell( varargs, 0 );
    mxArray* vals = mxGetCell( varargs, 1 );

    BufferString nmstr = mxArrayToString( pars );
    BufferString valstr; valstr = mxGetScalar( vals );

    names.add( nmstr );
    values.add( valstr );

    /*
    const int sz = mxGetN( varargs );
    for ( int idx=0; idx<sz; idx++ )
    {
	BufferString str(80,true);
	mxGetString( varargs, str.buf(), 80 );
	names.add( str );
	values.add( str );
    }
    */

    return res;
}

#else
bool MatlabLibAccess::init()		{ return true; }
bool MatlabLibAccess::terminate()	{ return true; }
bool MatlabLibAccess::getParameters( BufferStringSet& names,
				     BufferStringSet& values ) const
{ return true; }
#endif


void* MatlabLibAccess::getFunction( const char* funcnm ) const
{
    const SharedLibAccess* sla = MLM().getSharedLibAccess( shlibfnm_ );
    if ( !sla ) return 0;

    void* fn = sla->getFunction( funcnm );
    if ( !fn )
	errmsg_ = BufferString("Cannot find function ",funcnm);

    return fn;
}



// MatlabLibMgr
MatlabLibMgr::MatlabLibMgr()
    : inited_(false)
{}


MatlabLibMgr::~MatlabLibMgr()
{}


bool MatlabLibMgr::load( const char* libfnm )
{
    errmsg_ = "";
    const FilePath fp( libfnm );
    const BufferString libnm = fp.fileName();
    if ( isLoaded(libnm) )
	return true;

    SharedLibAccess* sla = new SharedLibAccess( libfnm );
    if ( !sla->isOK() )
	return false;

    slas_ += sla;
    libnms_.add( libnm );
    return true;
}


bool MatlabLibMgr::isLoaded( const char* libfnm ) const
{ return getSharedLibAccess( libfnm ); }


const SharedLibAccess*
    MatlabLibMgr::getSharedLibAccess( const char* libfnm ) const
{
    const FilePath fp( libfnm );
    const BufferString libnm = fp.fileName();
    const int idx = libnms_.indexOf( libnm );
    return slas_.validIdx(idx) ? slas_[idx] : 0;
}


#ifdef HAS_MATLAB
bool MatlabLibMgr::initApplication()
{
    if ( inited_ ) return true;

    mclmcrInitialize();
    if ( !mclInitializeApplication(NULL,0) )
    {
	errmsg_ = "Cannot initialize MATLAB application";
	return false;
    }

    inited_ = true;
    return true;
}


void MatlabLibMgr::terminateApplication()
{
    if ( !inited_ ) return;

    mclTerminateApplication();
    inited_ = false;
}
#else

bool MatlabLibMgr::initApplication()		{ return false; }
void MatlabLibMgr::terminateApplication()	{}

#endif

MatlabLibMgr& MLM()
{
    static PtrMan<MatlabLibMgr> inst = new MatlabLibMgr;
    return *inst;
}
