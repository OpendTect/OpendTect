/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Aug 2007
 RCS:           $Id: emmarchingcubessurface.cc,v 1.5 2008-09-06 14:34:41 cvskris Exp $
________________________________________________________________________

-*/

#include "emmarchingcubessurface.h"
#include "emmarchingcubessurfacetr.h"

#include "arrayndimpl.h"
#include "ascstream.h"
#include "datainterp.h"
#include "datachar.h"
#include "executor.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "marchingcubes.h"
#include "randcolor.h"
#include "streamconn.h"

#include "emmanager.h"

namespace EM 
{

class MarchingCubesSurfaceReader : public Executor
{
public:
    ~MarchingCubesSurfaceReader()
    {
	delete conn_;
	delete int32interpreter_;
	delete exec_;
    }

    MarchingCubesSurfaceReader( EM::MarchingCubesSurface& surface, Conn* conn )
	: Executor( "Surface Loader" )
	, conn_( conn )
	, int32interpreter_( 0 )
	, exec_( 0 )
	, surface_( surface )
    {
	if ( !conn_ || !conn_->forRead() )
	{
	    errmsg_ = "Cannot open connection";
	    return;
	}

	std::istream& strm = ((StreamConn*)conn_)->iStream();
	ascistream astream( strm );
	if ( !astream.isOfFileType( sFileType() ) )
	{
	    errmsg_ = "Invalid filetype";
	    return;
	}

	IOPar par( astream );

	BufferString dcs;
	if ( par.get( sKeyInt32DataChar(), dcs ) )
	{
	    DataCharacteristics dc; dc.set( dcs.buf() );
	    int32interpreter_ = new DataInterpreter<int>( dc );
	}

	SamplingData<int> inlsampling;
	SamplingData<int> crlsampling;
	SamplingData<float> zsampling;
	if ( !par.get( sKeyInlSampling(),inlsampling.start,inlsampling.step) ||
	     !par.get( sKeyCrlSampling(),crlsampling.start,crlsampling.step) ||
	     !par.get( sKeyZSampling(),zsampling.start,zsampling.step ) )
	{
	    errmsg_ = "Invalid filetype";
	    return;
	}

	surface.setInlSampling( inlsampling );
	surface.setCrlSampling( crlsampling );
	surface.setZSampling( zsampling );

	Color col;
	if ( par.get( sKey::Color, col ) )
	    surface.setPreferredColor( col );

	exec_ = surface.surface().readFrom(strm,int32interpreter_);
    }

    static const char* sKeyInt32DataChar()	{ return "Int32 format"; }
    static const char* sKeyInlSampling()	{ return "Inline sampling"; }
    static const char* sKeyCrlSampling()	{ return "Crossline sampling"; }
    static const char* sKeyZSampling()		{ return "Z sampling"; }
    static const char* sFileType()
    { return EM::MarchingCubesSurface::typeStr(); }

    int	nextStep()
    {
	if ( !exec_ ) return ErrorOccurred;
	const int res = exec_->doStep();
	if ( res==Finished )
	{
	    surface_.setFullyLoaded( true );
	    surface_.resetChangedFlag();
	}
	return res;
    }

    int	totalNr() const { return exec_ ? exec_->totalNr() : -1; }
    int	nrDone() const { return exec_ ? exec_->nrDone() : -1; }
    const char*	nrDoneText() const { return exec_ ? exec_->nrDoneText() : 0; }
protected:

    EM::MarchingCubesSurface&	surface_;
    Executor*			exec_;
    DataInterpreter<od_int32>*	int32interpreter_;
    Conn*			conn_;
    BufferString		errmsg_;
};



class MarchingCubesSurfaceWriter : public Executor
{
public:
~MarchingCubesSurfaceWriter()
{
    delete conn_;
    delete exec_;
}

MarchingCubesSurfaceWriter( EM::MarchingCubesSurface& surface,
			    Conn* conn, bool binary )
    : Executor( "Surface Writer" )
    , conn_( conn )
    , exec_( 0 )
    , surface_( surface )
{
    if ( !conn_ || !conn_->forWrite() )
    {
	errmsg_ = "Cannot open connection";
	return;
    }

    std::ostream& strm = ((StreamConn*)conn_)->oStream();
    ascostream astream( strm );
    astream.putHeader( MarchingCubesSurfaceReader::sFileType());

    IOPar par;
    if ( binary )
    {
	BufferString dcs;
	od_int32 dummy;
	DataCharacteristics(dummy).toString( dcs.buf() );
	par.set(MarchingCubesSurfaceReader::sKeyInt32DataChar(),
		dcs.buf() );
    }

    par.set( MarchingCubesSurfaceReader::sKeyInlSampling(),
	     surface.inlSampling().start,surface.inlSampling().step);
    par.set( EM::MarchingCubesSurfaceReader::sKeyCrlSampling(),
	     surface.crlSampling().start,surface.crlSampling().step);
    par.set( EM::MarchingCubesSurfaceReader::sKeyZSampling(),
	     surface.zSampling().start,surface.zSampling().step );

    par.set( sKey::Color, surface.preferredColor() );

    par.putTo( astream );

    exec_ = surface.surface().writeTo( strm, binary );
}

int nextStep()
{
    if ( !exec_ ) return ErrorOccurred;
    const int res = exec_->doStep();
    if ( !res )
	surface_.resetChangedFlag();

    return res;
}

int totalNr() const { return exec_ ? exec_->totalNr() : -1; }
int nrDone() const { return exec_ ? exec_->nrDone() : -1; }
const char* nrDoneText() const { return exec_ ? exec_->nrDoneText() : 0; }

protected:

    Executor*			exec_;
    Conn*			conn_;
    BufferString		errmsg_;
    EM::MarchingCubesSurface&	surface_;
};



mImplementEMObjFuncs( EM::MarchingCubesSurface, "MarchingCubesSurface" );


EM::MarchingCubesSurface::MarchingCubesSurface( EM::EMManager& emm )
    : EMObject( emm )
    , mcsurface_( new ::MarchingCubesSurface )
{
    mcsurface_->ref();
    setPreferredColor( getRandomColor( false ) );
}


EM::MarchingCubesSurface::~MarchingCubesSurface()
{
    mcsurface_->unRef();
}



const Geometry::Element*
EM::MarchingCubesSurface::sectionGeometry( const SectionID& sid ) const
{
    return 0;
}


Geometry::Element* EM::MarchingCubesSurface::sectionGeometry( const SectionID& sid )
{
    return 0;
}


Executor* EM::MarchingCubesSurface::loader()
{
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    if ( !ioobj )
	return 0;

    Conn* conn = ioobj->getConn( Conn::Read );
    if ( !conn )
	return 0;

    return new EM::MarchingCubesSurfaceReader( *this, conn );
}


Executor* EM::MarchingCubesSurface::saver()
{
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    if ( !ioobj )
	return 0;

    Conn* conn = ioobj->getConn( Conn::Write );
    if ( !conn )
	return 0;

    return new MarchingCubesSurfaceWriter( *this, conn, false );
}


bool EM::MarchingCubesSurface::isEmpty() const
{ return mcsurface_->isEmpty(); }


const IOObjContext& EM::MarchingCubesSurface::getIOObjContext() const
{ return EMMarchingCubesSurfaceTranslatorGroup::ioContext(); }


ImplicitBody*
EM::MarchingCubesSurface::createImplicitBody( TaskRunner* tr ) const
{
    if ( !mcsurface_ )
	return 0;

    mcsurface_->modelslock_.readLock();
    Interval<int> inlrg, crlrg, zrg;
    if ( !mcsurface_->models_.getRange( 0, inlrg ) ||
	!mcsurface_->models_.getRange( 1, crlrg ) ||
	!mcsurface_->models_.getRange( 2, zrg ) )
    {
	mcsurface_->modelslock_.readUnLock();
	return 0;
    }

    const int inlsz = inlrg.width()+1;
    const int crlsz = crlrg.width()+1;
    const int zsz = zrg.width()+1;


    mDeclareAndTryAlloc(Array3D<int>*,intarr,Array3DImpl<int>(inlsz,crlsz,zsz));
    if ( !intarr )
    {
	mcsurface_->modelslock_.readUnLock();
	return 0;
    }

    mDeclareAndTryAlloc( ImplicitBody*, res, ImplicitBody );
    if ( !res )
    {
	mcsurface_->modelslock_.readUnLock();
	delete intarr;
	return 0;
    }
    Array3D<float>* arr = new Array3DConv<float,int>(intarr);
    if ( !arr )
    {
	mcsurface_->modelslock_.readUnLock();
	delete intarr;
	delete res;
	return 0;
    }

    res->arr_ = arr;
    res->inlsampling_.start = inlsampling_.atIndex( inlrg.start );
    res->inlsampling_.step = inlsampling_.step;

    res->crlsampling_.start = crlsampling_.atIndex( crlrg.start );
    res->crlsampling_.step = crlsampling_.step;

    res->zsampling_.start = zsampling_.atIndex( zrg.start );
    res->zsampling_.step = zsampling_.step;

    MarchingCubes2Implicit m2i( *mcsurface_, *intarr,
				inlrg.start, crlrg.start, zrg.start );
    if ( !m2i.compute() )
    {
	delete res;
	mcsurface_->modelslock_.readUnLock();
	return 0;
    }

    res->threshold_ = m2i.threshold();
    mcsurface_->modelslock_.readUnLock();
    return res;
}



bool  EM::MarchingCubesSurface::setSurface( ::MarchingCubesSurface* ns )
{
    if ( !ns )
	return false;

    mcsurface_->unRef();
    mcsurface_ = ns;
    mcsurface_->ref();

    return true;
}


void EM::MarchingCubesSurface::setInlSampling( const SamplingData<int>& s )
{ inlsampling_ = s; }


void EM::MarchingCubesSurface::setCrlSampling( const SamplingData<int>& s )
{ crlsampling_ = s; }


void EM::MarchingCubesSurface::setZSampling( const SamplingData<float>& s )
{ zsampling_ = s; }


}; // namespace
