/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emmarchingcubessurface.h"

#include "arrayndimpl.h"
#include "ascstream.h"
#include "datachar.h"
#include "datainterp.h"
#include "embodyoperator.h"
#include "embodytr.h"
#include "emmanager.h"
#include "executor.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "marchingcubes.h"
#include "randcolor.h"
#include "separstr.h"
#include "streamconn.h"
#include "uistrings.h"
#include "unitofmeasure.h"

namespace EM
{

class MarchingCubesSurfaceReader : public Executor
{ mODTextTranslationClass(MarchingCubesSurfaceReader);
public:
    ~MarchingCubesSurfaceReader()
    {
	delete conn_;
	delete int32interpreter_;
	delete exec_;
    }

    MarchingCubesSurfaceReader( MarchingCubesSurface& surface, Conn* conn )
	: Executor( "Surface Loader" )
	, conn_( conn )
	, int32interpreter_( 0 )
	, exec_( 0 )
	, surface_( surface )
    {
	if ( !conn_ || !conn_->forRead() )
	{
	    errmsg_ = uiStrings::phrCannotOpen( uiStrings::sGeobody() );
	    return;
	}

	od_istream& strm = ((StreamConn*)conn_)->iStream();
	ascistream astream( strm );
	if ( !astream.isOfFileType( sFileType() ) &&
	     !astream.isOfFileType( sOldFileType() ) &&
	     !astream.isOfFileType( sOldFileType2() ) )
	{
	    errmsg_ = ::toUiString("Invalid filetype");
	    return;
	}

	IOPar par( astream );

	int32interpreter_ =
	    DataInterpreter<int>::create( par, sKeyInt32DataChar(), true );

	SamplingData<int> inlsampling;
	SamplingData<int> crlsampling;
	SamplingData<float> zsampling;
        if ( !par.get( sKeyInlSampling(),inlsampling.start_,inlsampling.step_) ||
             !par.get( sKeyCrlSampling(),crlsampling.start_,crlsampling.step_) ||
             !par.get( sKeyZSampling(),zsampling.start_,zsampling.step_ ) )
	{
	    errmsg_ = ::toUiString("Invalid filetype");
	    return;
	}

	surface.setInlSampling( inlsampling );
	surface.setCrlSampling( crlsampling );
	surface.setZSampling( zsampling );
	surface.usePar( par );

	exec_ = surface.surface().readFrom(strm,int32interpreter_);
    }

    static const char* sKeyInt32DataChar()	{ return "Int32 format"; }
    static const char* sKeyInlSampling()	{ return "Inline sampling"; }
    static const char* sKeyCrlSampling()	{ return "Crossline sampling"; }
    static const char* sKeyZSampling()		{ return "Z sampling"; }
    static const char* sFileType()
    { return MarchingCubesSurface::typeStr(); }

    static const char* sOldFileType()	{ return "MarchingCubesSurface"; }
    static const char* sOldFileType2()	{ return "MCBody"; }

    int nextStep() override
    {
	return exec_ ? exec_->doStep() : ErrorOccurred();
    }

    bool doFinish( bool success, od_ostream* ) override
    {
	if ( success )
	{
	    surface_.setFullyLoaded( true );
	    surface_.resetChangedFlag();
	}

	return success;
    }

    od_int64	totalNr() const override
    { return exec_ ? exec_->totalNr() : -1; }

    od_int64	nrDone() const override
    { return exec_ ? exec_->nrDone() : -1; }

    uiString	uiNrDoneText() const override
    { return exec_ ? exec_->uiNrDoneText() : uiString::emptyString(); }

    uiString	uiMessage() const override
    {
	return errmsg_.isEmpty()
	    ? tr("Loading body")
	    : errmsg_;
    }

protected:

    MarchingCubesSurface&	surface_;
    Executor*			exec_;
    DataInterpreter<od_int32>*	int32interpreter_;
    Conn*			conn_;
    uiString			errmsg_;
};



class MarchingCubesSurfaceWriter : public Executor
{ mODTextTranslationClass(MarchingCubesSurfaceWriter);
public:
~MarchingCubesSurfaceWriter()
{
    delete conn_;
    delete exec_;
}

MarchingCubesSurfaceWriter( MarchingCubesSurface& surface,
			    Conn* conn, bool binary )
    : Executor( "Surface Writer" )
    , conn_( conn )
    , exec_( 0 )
    , surface_( surface )
{
    if ( !conn_ || !conn_->forWrite() )
    {
	errmsg_ = uiStrings::phrCannotOpen( uiStrings::sGeobody() );
	return;
    }

    od_ostream& strm = ((StreamConn*)conn_)->oStream();
    ascostream astream( strm );
    astream.putHeader( MarchingCubesSurfaceReader::sFileType());

    IOPar par;
    if ( binary )
    {
	BufferString dcs;
	od_int32 dummy;
	DataCharacteristics(dummy).toString( dcs );
	par.set(MarchingCubesSurfaceReader::sKeyInt32DataChar(),
		dcs.buf() );
    }

    par.set( MarchingCubesSurfaceReader::sKeyInlSampling(),
             surface.inlSampling().start_,surface.inlSampling().step_);
    par.set( MarchingCubesSurfaceReader::sKeyCrlSampling(),
             surface.crlSampling().start_,surface.crlSampling().step_);
    par.set( MarchingCubesSurfaceReader::sKeyZSampling(),
             surface.zSampling().start_,surface.zSampling().step_ );

    par.set( sKey::Color(), surface.preferredColor() );
    par.set( sKey::ZUnit(),
	     UnitOfMeasure::surveyDefZStorageUnit()->name() );

    par.putTo( astream );

    exec_ = surface.surface().writeTo( strm, binary );
}

int nextStep() override
{
    if ( !exec_ ) return ErrorOccurred();
    const int res = exec_->doStep();
    if ( !res )
	surface_.resetChangedFlag();

    return res;
}

od_int64 totalNr() const override { return exec_ ? exec_->totalNr() : -1; }
od_int64 nrDone() const override { return exec_ ? exec_->nrDone() : -1; }
uiString uiNrDoneText() const override { return exec_ ? exec_->uiNrDoneText()
					     : uiString::emptyString(); }
uiString uiMessage() const override
{
    return errmsg_.isEmpty()
	? tr("Loading body")
	: errmsg_;
}


protected:

    Executor*			exec_;
    Conn*			conn_;
    uiString			errmsg_;
    MarchingCubesSurface&	surface_;
};


void MarchingCubesSurface::initClass()
{
    SeparString sep( 0, FactoryBase::cSeparator() );
    sep += typeStr();
    sep += MarchingCubesSurfaceReader::sOldFileType();
    EMOF().addCreator( create, sep.buf() );
}


EMObject* MarchingCubesSurface::create( EMManager& emm ) \
{
    EMObject* obj = new MarchingCubesSurface( emm );
    if ( !obj )
	return nullptr;

    obj->ref();
    emm.addObject( obj );
    obj->unRefNoDelete();
    return obj;
}


StringView MarchingCubesSurface::typeStr()
{ return "MC"; }


const char* MarchingCubesSurface::getTypeStr() const
{ return typeStr(); }


void MarchingCubesSurface::setNewName()
{
    mDefineStaticLocalObject( int, objnr, = 1 );
    BufferString nm( "<New Geobody " );
    nm.add( objnr++ ).add( ">" );
    setName( nm );
}


MarchingCubesSurface::MarchingCubesSurface( EMManager& emm )
    : EMObject( emm )
    , mcsurface_( new ::MarchingCubesSurface )
    , operator_( 0 )
{
    mcsurface_->ref();
    setPreferredColor( OD::getRandomColor( false ) );
}


MarchingCubesSurface::~MarchingCubesSurface()
{
    mcsurface_->unRef();
    delete operator_;
}


void MarchingCubesSurface::refBody()
{
    EMObject::ref();
}


void MarchingCubesSurface::unRefBody()
{
    EMObject::unRef();
}


MultiID MarchingCubesSurface::storageID() const
{ return EMObject::multiID(); }


BufferString MarchingCubesSurface::storageName() const
{ return EMObject::name(); }


Executor* MarchingCubesSurface::loader()
{
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    Conn* conn = ioobj ? ioobj->getConn( Conn::Read ) : 0;
    return conn ? new MarchingCubesSurfaceReader( *this, conn ) : 0;
}


Executor* MarchingCubesSurface::saver()
{ return saver(0); }


Executor* MarchingCubesSurface::saver( const IOObj* inpioobj )
{
    PtrMan<IOObj> ioobj = nullptr;
    if ( inpioobj )
	ioobj = inpioobj->clone();
    else
	ioobj = IOM().get( multiID() );

    if ( !ioobj )
	return nullptr;

    ioobj->pars().set( sKey::Type(), getTypeStr() );
    IOM().commitChanges( *ioobj );
    Conn* conn = ioobj->getConn( Conn::Write );
    return conn ? new MarchingCubesSurfaceWriter( *this, conn, true ) : 0;
}


bool MarchingCubesSurface::isEmpty() const
{ return mcsurface_->isEmpty(); }


const IOObjContext& MarchingCubesSurface::getIOObjContext() const
{
    mDefineStaticLocalObject( PtrMan<IOObjContext>, res, = 0 );
    if ( !res )
    {
	IOObjContext* newres =
		new IOObjContext(EMBodyTranslatorGroup::ioContext() );
	newres->fixTranslator( typeStr() );

	res.setIfNull(newres,true);
    }

    return *res;
}


bool MarchingCubesSurface::useBodyPar( const IOPar& par )
{ return EMObject::usePar( par ); }


void MarchingCubesSurface::fillBodyPar( IOPar& par ) const
{ EMObject::fillPar( par ); }


void MarchingCubesSurface::setBodyOperator( BodyOperator* op )
{
    if ( operator_ ) delete operator_;
    operator_ = op;
}


void MarchingCubesSurface::createBodyOperator()
{
    if ( operator_ ) delete operator_;
    operator_ = new BodyOperator();
}


bool MarchingCubesSurface::regenerateMCBody( TaskRunner* taskrunner )
{
    if ( !operator_ || !mcsurface_ )
	return false;

    ImplicitBody* body = 0;
    if ( !operator_->createImplicitBody(body,taskrunner) || !body )
	return false;

    setInlSampling( SamplingData<int>(body->tkzs_.hsamp_.inlRange()) );
    setCrlSampling( SamplingData<int>(body->tkzs_.hsamp_.crlRange()) );
    setZSampling( SamplingData<float>(body->tkzs_.zsamp_) );

    return mcsurface_->setVolumeData( 0, 0, 0, *body->arr_, body->threshold_ );
}


bool MarchingCubesSurface::getBodyRange( TrcKeyZSampling& cs )
{
    Interval<int> inlrg, crlrg, zrg;
    if ( !mcsurface_->models_.getRange( 0, inlrg ) ||
	 !mcsurface_->models_.getRange( 1, crlrg ) ||
	 !mcsurface_->models_.getRange( 2, zrg ) )
	return false;

    cs.hsamp_.start_ = BinID( inlsampling_.atIndex(inlrg.start_),
                              crlsampling_.atIndex(crlrg.start_) );
    cs.hsamp_.stop_ = BinID( inlsampling_.atIndex(inlrg.stop_),
                             crlsampling_.atIndex(crlrg.stop_) );
    cs.hsamp_.step_ = BinID( inlsampling_.step_, crlsampling_.step_ );
    cs.zsamp_.start_ = zsampling_.atIndex( zrg.start_ );
    cs.zsamp_.step_ = zsampling_.step_;
    cs.zsamp_.stop_ = zsampling_.atIndex( zrg.stop_ );

    return true;
}


ImplicitBody* MarchingCubesSurface::createImplicitBody( TaskRunner* taskrunner,
							bool smooth ) const
{
    if ( !mcsurface_ )
    {
	if ( operator_ )
	{
	    ImplicitBody* body = 0;
	    if ( operator_->createImplicitBody(body,taskrunner) && body )
		return body;
	}

	return 0;
    }

    Threads::Locker lckr( mcsurface_->modelslock_ );
    Interval<int> inlrg, crlrg, zrg;
    if ( !mcsurface_->models_.getRange( 0, inlrg ) ||
	!mcsurface_->models_.getRange( 1, crlrg ) ||
	!mcsurface_->models_.getRange( 2, zrg ) )
	return 0;

    const int inlsz = inlrg.width()+1;
    const int crlsz = crlrg.width()+1;
    const int zsz = zrg.width()+1;

    mDeclareAndTryAlloc( ImplicitBody*, res, ImplicitBody );
    if ( !res ) return 0;

    mDeclareAndTryAlloc(Array3D<int>*,intarr,Array3DImpl<int>(inlsz,crlsz,zsz));
    if ( !intarr )
    {
	delete res; return 0;
    }

    MarchingCubes2Implicit m2i( *mcsurface_, *intarr,
                                inlrg.start_, crlrg.start_, zrg.start_, !smooth );
    const bool execres = TaskRunner::execute( taskrunner, m2i );
    if ( !execres )
    {
	delete res; return 0;
    }

    Array3D<float>* arr = new Array3DConv<float,int>(intarr);
    if ( !arr )
    {
	delete intarr; delete res; return 0;
    }

    res->arr_ = arr;
    res->threshold_ = m2i.threshold();

    res->tkzs_.hsamp_.start_ = BinID( inlsampling_.atIndex(inlrg.start_),
                                      crlsampling_.atIndex(crlrg.start_) );
    res->tkzs_.hsamp_.step_ = BinID( inlsampling_.step_, crlsampling_.step_ );
    res->tkzs_.hsamp_.stop_ = BinID( inlsampling_.atIndex(inlrg.stop_),
                                     crlsampling_.atIndex(crlrg.stop_) );
    res->tkzs_.zsamp_.start_ = zsampling_.atIndex( zrg.start_ );
    res->tkzs_.zsamp_.stop_ = zsampling_.atIndex( zrg.stop_ );
    res->tkzs_.zsamp_.step_ = zsampling_.step_;

    return res;
}


bool MarchingCubesSurface::setSurface( ::MarchingCubesSurface* ns )
{
    if ( !ns )
	return false;

    mcsurface_->unRef();
    mcsurface_ = ns;
    mcsurface_->ref();

    return true;
}


void MarchingCubesSurface::setInlSampling( const SamplingData<int>& s )
{ inlsampling_ = s; }


void MarchingCubesSurface::setCrlSampling( const SamplingData<int>& s )
{ crlsampling_ = s; }


void MarchingCubesSurface::setZSampling( const SamplingData<float>& s )
{ zsampling_ = s; }


void MarchingCubesSurface::setSampling( const TrcKeyZSampling& tkzs )
{
    setInlSampling( SamplingData<int>(tkzs.hsamp_.start_.inl(),
				      tkzs.hsamp_.step_.inl()) );
    setCrlSampling( SamplingData<int>(tkzs.hsamp_.start_.crl(),
				      tkzs.hsamp_.step_.crl()) );
    setZSampling( SamplingData<float>(tkzs.zsamp_.start_,tkzs.zsamp_.step_) );
}

} // namespace EM
