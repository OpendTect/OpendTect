/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emsurfacetr.h"

#include "dirlist.h"
#include "emfaultauxdata.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurface.h"
#include "emsurfaceio.h"
#include "emsurfauxdataio.h"
#include "emhorizon3d.h"
#include "emfaultset3d.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "strmprov.h"
#include "survinfo.h"
#include "settings.h"
#include "uistrings.h"


uiString EMHorizon3DTranslatorGroup::sTypeName(int num)
{ return SI().has2D() ? mJoinUiStrs(s3D(), sHorizon(num))
		      : uiStrings::sHorizon(num); }

mDefSimpleTranslatorSelector(EMHorizon3D)

const IOObjContext& EMHorizon3DTranslatorGroup::ioContext()
{
    mDefineStaticLocalObject( PtrMan<IOObjContext>, ctxt, (0) );
    if ( !ctxt )
    {
	IOObjContext* newctxt = new IOObjContext( 0 );
	newctxt->stdseltype_ = IOObjContext::Surf;
	newctxt->trgroup_ = &theInst();

        ctxt.setIfNull( newctxt, true );
    }

    return *ctxt;
}


uiString EMHorizon2DTranslatorGroup::sTypeName(int num)
{ return mJoinUiStrs(s2D(), sHorizon(num)); }

mDefSimpleTranslatorSelector(EMHorizon2D)

const IOObjContext& EMHorizon2DTranslatorGroup::ioContext()
{
    mDefineStaticLocalObject( PtrMan<IOObjContext>, ctxt, (0) );
    if ( !ctxt )
    {
	IOObjContext* newctxt = new IOObjContext( 0 );
	newctxt->stdseltype_ = IOObjContext::Surf;
	newctxt->trgroup_ = &theInst();

        ctxt.setIfNull( newctxt, true );
    }

    return *ctxt;
}


uiString EMAnyHorizonTranslatorGroup::sTypeName(int)
{ return tr("Any Horizon"); }

mDefSimpleTranslatorioContext(EMAnyHorizon,Surf)

int EMAnyHorizonTranslatorGroup::selector( const char* s )
{
    int retval3d = defaultSelector(EMHorizon3DTranslatorGroup::sGroupName(), s);
    int retval2d = defaultSelector(EMHorizon2DTranslatorGroup::sGroupName(), s);
    return retval3d ? retval3d : retval2d;
}


uiString EMFault3DTranslatorGroup::sTypeName(int num)
{ return uiStrings::sFault(num); }

mDefSimpleTranslatorSelector(EMFault3D)

mDefSimpleTranslatorioContext(EMFault3D,Surf)

uiString EMFaultSet3DTranslatorGroup::sTypeName(int num)
{ return uiStrings::sFaultSet(num); }

mDefSimpleTranslatorSelector(EMFaultSet3D)

mDefSimpleTranslatorioContext(EMFaultSet3D,Surf)


uiString EMFaultStickSetTranslatorGroup::sTypeName( int num )
{ return uiStrings::sFaultStickSet( num ); }

mDefSimpleTranslatorSelector(EMFaultStickSet)
mDefSimpleTranslatorioContext(EMFaultStickSet,Surf)


EMSurfaceTranslator::~EMSurfaceTranslator()
{
    delete ioobj_;
}


void EMSurfaceTranslator::init( const EM::Surface* surf, const IOObj* ioobj )
{
    surface_ = const_cast<EM::Surface*>(surf);
    setIOObj( ioobj );
}


void EMSurfaceTranslator::setIOObj( const IOObj* ioobj )
{
    delete ioobj_;
    ioobj_ = ioobj ? ioobj->clone() : 0;
}


bool EMSurfaceTranslator::startRead( const IOObj& ioobj )
{
    init( nullptr, &ioobj );
    if ( !prepRead() )
	return false;

    sels_.setDefault();
    return true;
}


bool EMSurfaceTranslator::startWrite( const EM::Surface& surf )
{
    init( &surf, nullptr );
    sd_.use( surf );
    if ( !prepWrite() )
	return false;

    sels_.setDefault();
    return true;
}


Executor* EMSurfaceTranslator::writer( const IOObj& ioobj, bool fullremove )
{
    setIOObj( &ioobj );
    Executor* ret = getWriter();
    if ( fullremove && ret )
	IOM().implRemove( ioobj );

    return ret;
}


#define mImplStart(fn) \
    if ( !ioobj || ioobj->translator()!="dGB" ) return false; \
    const BufferString basefnm( ioobj->fullUserExpr(true) ); \
    StreamProvider sp( basefnm.buf() ); \
    FilePath basefp( basefnm ); \
    const BufferString pathnm( basefp.pathOnly() ); \
    bool res = sp.fn;

#define mImplLoopStart \
    if ( gap > 100 ) break; \
    StreamProvider loopsp( EM::dgbSurfDataWriter::createHovName( \
                                                      basefnm.buf(),nr).buf() )


bool EMSurfaceTranslator::implRemove( const IOObj* ioobj, bool ) const
{
    if ( ioobj )
    {
	EM::IOObjInfo ioinfo( *ioobj );
	if ( ioinfo.type() == EM::IOObjInfo::Fault )
	{
	    EM::FaultAuxData fad( ioobj->key() );
	    fad.removeAllData();
	}
    }

    mImplStart(remove(false));

    int gap = 0;
    for ( int nr=0; ; nr++ )
    {
	mImplLoopStart;
	if ( loopsp.exists(true) )
	    loopsp.remove(false);
	else
	    gap++;
    }

#define mRemove( fn ) { \
    StreamProvider parsp( EM::Surface::fn(*ioobj).buf() ); \
    parsp.addPathIfNecessary( pathnm.buf() ); \
    if ( parsp.exists(true) ) \
	parsp.remove(false); }

    mRemove( getSetupFileName )
    mRemove( getParFileName )
    mRemove( getParentChildFileName )

    return res;
}


bool EMSurfaceTranslator::implRename( const IOObj* ioobj,
				      const char* newnm ) const
{
    if ( ioobj )
    {
	EM::IOObjInfo ioinfo( ioobj );
	if ( ioinfo.type() == EM::IOObjInfo::Fault )
	{
	    FilePath fp( newnm );
	    fp.setExtension("");
	    EM::FaultAuxData fad( ioobj->key() );
	    fad.renameFault( fp.fileName() );
	}
    }

    mImplStart(rename(newnm));

    int gap = 0;
    for ( int nr=0; ; nr++ )
    {
	mImplLoopStart;
	StreamProvider spnew(
		EM::dgbSurfDataWriter::createHovName(newnm,nr).buf() );
	spnew.addPathIfNecessary( pathnm.buf() );
	if ( loopsp.exists(true) )
	    loopsp.rename( spnew.fileName() );
	else
	    gap++;
    }

#define mRename( fn ) { \
    FilePath fp( EM::Surface::fn(*ioobj).buf() ); \
    StreamProvider oldsp( fp.fullPath().buf() ); \
    oldsp.addPathIfNecessary( pathnm.buf() ); \
    FilePath newfp( newnm ); \
    newfp.setExtension( fp.extension() ); \
    if ( oldsp.exists(true) ) \
	oldsp.rename( newfp.fullPath().buf() ); }

    mRename( getSetupFileName )
    mRename( getParFileName )
    mRename( getParentChildFileName )

    return res;
}


bool EMSurfaceTranslator::implSetReadOnly( const IOObj* ioobj, bool ro ) const
{
    mImplStart(setReadOnly(ro));

    return res;
}


bool EMSurfaceTranslator::getBinarySetting()
{
    bool isbinary = true;
    mSettUse(getYN,"dTect.Surface","Binary format",isbinary);
    return isbinary;
}


dgbEMSurfaceTranslator::dgbEMSurfaceTranslator( const char* nm, const char* unm)
    : EMSurfaceTranslator(nm,unm)
    , reader_(0)
{
}


dgbEMSurfaceTranslator::~dgbEMSurfaceTranslator()
{
    delete reader_;
}


bool dgbEMSurfaceTranslator::prepRead()
{
    if ( reader_ )
	delete reader_;

    BufferString unm( group() ? group()->groupName().buf() : 0 );
    reader_ = new EM::dgbSurfaceReader( *ioobj_, unm.buf() );
    if ( !reader_->isOK() )
    {
	errmsg_ = reader_->uiMessage();
	return false;
    }

    sd_.sections.add( "[0]" );

    for ( int idx=0; idx<reader_->nrAuxVals(); idx++ )
    {
	sd_.valnames.add( reader_->auxDataName(idx) );
	sd_.valshifts_ += reader_->auxDataShift(idx);
    }

    for ( int idx=0; idx<reader_->nrLines(); idx++ )
    {
	const Pos::GeomID geomid = reader_->lineGeomID( idx );
	const BufferString linenm = reader_->lineName( idx );
	if ( mIsUdfGeomID(geomid) || linenm.isEmpty() )
	    continue;

	StepInterval<int> trcrange = reader_->lineTrcRanges(idx);
	if ( !mIsUdf(trcrange.start) && !mIsUdf(trcrange.stop) )
	{
	    sd_.linenames.add( linenm );
	    sd_.geomids.add( geomid );
	    sd_.trcranges += reader_->lineTrcRanges(idx);
	}
    }

    const int version = reader_->version();
    if ( version==1 )
    {
	sd_.rg.start_ = SI().sampling(false).hsamp_.start_;
	sd_.rg.stop_ = SI().sampling(false).hsamp_.stop_;
	sd_.rg.step_ = SI().sampling(false).hsamp_.step_;
    }
    else
    {
	sd_.rg.start_.inl() = reader_->rowInterval().start;
	sd_.rg.stop_.inl() = reader_->rowInterval().stop;
	sd_.rg.step_.inl() = reader_->rowInterval().step;
	sd_.rg.start_.crl() = reader_->colInterval().start;
	sd_.rg.stop_.crl() = reader_->colInterval().stop;
	sd_.rg.step_.crl() = reader_->colInterval().step;
	sd_.zrg.start = reader_->zInterval().start;
	sd_.zrg.stop = reader_->zInterval().stop;
	sd_.zunit = reader_->zUnit();

	sd_.dbinfo = reader_->dbInfo();
    }

    sd_.nrfltsticks_ = reader_->rowInterval().width()+1;
    reader_->setReadOnlyZ( readOnlyZ() );

    return true;
}


void dgbEMSurfaceTranslator::getSels( StepInterval<int>& rrg,
				      StepInterval<int>& crg )
{
    if ( sels_.rg.isEmpty() )
	sels_.rg = sd_.rg;

    rrg.start = sels_.rg.start_.inl(); rrg.stop = sels_.rg.stop_.inl();
    rrg.step = sels_.rg.step_.inl();
    crg.start = sels_.rg.start_.crl(); crg.stop = sels_.rg.stop_.crl();
    crg.step = sels_.rg.step_.crl();
}


Executor* dgbEMSurfaceTranslator::reader( EM::Surface& surf )
{
    surface_ = &surf;
    Executor* res = reader_;
    if ( reader_ )
    {
	reader_->setOutput( surf );
	if ( hasRangeSelection() )
	{
	    StepInterval<int> rrg, crg; getSels( rrg, crg );
	    reader_->setRowInterval( rrg ); reader_->setColInterval( crg );
	}

	if ( !sels_.sellinenames.isEmpty() && !sels_.seltrcranges.isEmpty() )
	{
	    reader_->setLineNames( sels_.sellinenames );
	    reader_->setLinesTrcRngs( sels_.seltrcranges );
	}

	reader_->selAuxData( sels_.selvalues );
    }

    reader_ = 0;
    return res;
}

Executor* dgbEMSurfaceTranslator::getWriter()
{
    BufferString unm( group() ? group()->groupName().buf() : 0 );
    EM::dgbSurfaceWriter* res =
	new EM::dgbSurfaceWriter(ioobj_,unm.buf(),
				 *surface_,getBinarySetting());
    res->setWriteOnlyZ( writeOnlyZ() );

    if ( hasRangeSelection() && !sels_.rg.isEmpty() )
    {
	StepInterval<int> rrg, crg; getSels( rrg, crg );
	res->setRowInterval( rrg ); res->setColInterval( crg );
    }

    if ( !sels_.selvalues.isEmpty() )
	res->selAuxData( sels_.selvalues );

    return res;
}


static BufferString getFileName( const char* fulluserexp, const char* attrnmptr)
{
    const StringView attrnm( attrnmptr );
    const BufferString basefnm( fulluserexp );
    BufferString fnm;
    for ( int idx=0, gap=0; gap<=100; idx++ )
    {
	fnm = EM::dgbSurfDataWriter::createHovName(basefnm,idx);
	if ( File::isEmpty(fnm.buf()) )
	    { gap++; continue; }

	const EM::dgbSurfDataReader rdr( fnm.buf() );
	if ( attrnm == rdr.dataName() )
	    return fnm;
    }

    return 0;
}

static BufferString getFileName( const IOObj& ioobj, const char* attrnm )
{ return getFileName( ioobj.fullUserExpr(true), attrnm ); }


Executor* dgbEMHorizon3DTranslator::getAuxdataReader( EM::Surface& surface,
							    int selidx )
{
    if ( selidx >= sels_.sd.valnames.size() )
	return 0;

    ExecutorGroup* grp = new ExecutorGroup( "Surface attributes reader" );
    for ( int idx=0; idx<sels_.sd.valnames.size(); idx++ )
    {
	if ( selidx>=0 && selidx != idx )
	    continue;

	const BufferString filenm = getFileName( *ioobj_,
					    sels_.sd.valnames[selidx]->buf() );
	if ( filenm.isEmpty() )
	    continue;

	EM::dgbSurfDataReader* rdr = new EM::dgbSurfDataReader( filenm.buf() );
	mDynamicCastGet(EM::Horizon3D*,hor3d,&surface)
	if ( !hor3d )
	    return 0;
	rdr->setSurface( *hor3d );
	grp->add( rdr );
    }

    return grp;
}


static BufferString getFreeFileName( const IOObj& ioobj )
{
    PtrMan<StreamConn> conn =
	dynamic_cast<StreamConn*>(ioobj.getConn(Conn::Read));
    if ( !conn ) return 0;

    const int maxnrfiles = 100000; // just a big number to make this loop end
    for ( int idx=0; idx<maxnrfiles; idx++ )
    {
	BufferString fnm =
	    EM::dgbSurfDataWriter::createHovName( conn->fileName(), idx );
	if ( !File::exists(fnm.buf()) )
	    return fnm;
    }

    return 0;
}


Executor* dgbEMHorizon3DTranslator::getAuxdataWriter(
			  const EM::Surface& surf, int dataidx, bool overwrite )
{
    mDynamicCastGet(const EM::Horizon3D*,hor3d,&surf)
    if ( !hor3d )
	return 0;

    bool isbinary = true;
    mSettUse(getYN,"dTect.Surface","Binary format",isbinary);

    ExecutorGroup* grp = new ExecutorGroup( "Surface Data saver" );
    grp->setNrDoneText( toUiString("Positions written") );
    BufferString fnm;
    for ( int selidx=0; selidx<sels_.sd.valnames.size(); selidx++ )
    {
	if ( dataidx >= 0 && dataidx != selidx )
	    continue;
	if ( overwrite )
	    fnm = getFileName( *ioobj_, sels_.sd.valnames.get(selidx) );
	if ( !overwrite || fnm.isEmpty() )
	    fnm = getFreeFileName( *ioobj_ );
	Executor* exec =
	    new EM::dgbSurfDataWriter(*hor3d,selidx,0,isbinary,fnm.buf());
	grp->add( exec );
    }

    return grp;
}



class dGBFaultSet3DReader : public Executor
{ mODTextTranslationClass(dGBFaultSet3DReader)
public:
dGBFaultSet3DReader( const IOObj& ioobj, EM::FaultSet3D& fltset )
    : Executor("Reading FaultSet")
    , fltset_(fltset)
    , curidx_(0)
    , dl_(ioobj.fullUserExpr(),File::FilesInDir,"*.flt")
{
    fltset_.setName( ioobj.name() );
    fltset_.setMultiID( ioobj.key() );
    IOPar disppars;
    EM::EMM().readDisplayPars( ioobj.key(), disppars );
    fltset_.useDisplayPar( disppars );
}

od_int64 nrDone() const override
{
    return curidx_;
}

od_int64 totalNr() const override
{
    return dl_.size();
}

uiString uiNrDoneText() const override
{
    return tr("Faults read");
}

int nextStep() override
{
    const int nrfaults = dl_.size();
    if ( curidx_ >= nrfaults )
       return Finished();

    const FilePath fp( dl_.fullPath(curidx_) );
    const EM::FaultID id( toInt( fp.baseName(), mUdf(int) ) );
    if ( !id.isValid() )
    {
	curidx_++;
	return MoreToDo();
    }

    BufferString fltnm( fltset_.name() );
    fltnm.add( "_" ).add( id.asInt() );

    mDynamicCastGet( EM::Fault3D*, newflt,
		     EM::EMM().createTempObject(EM::Fault3D::typeStr()) );
    EM::dgbSurfaceReader rdr( fp.fullPath(), fltnm,
			      mTranslGroupName(EMFault3D) );
    rdr.setOutput( *newflt );
    if ( !rdr.execute() )
    {
	curidx_++;
	return MoreToDo();
    }

    newflt->setName( fltnm );
    fltset_.addFault( newflt, id );
    curidx_++;
    return MoreToDo();
}

	EM::FaultSet3D&	fltset_;
	DirList		dl_;
	int		curidx_;

};



class dGBFaultSet3DWriter : public Executor
{ mODTextTranslationClass(dGBFaultSet3DWriter)
public:
dGBFaultSet3DWriter( const IOObj& ioobj, const EM::FaultSet3D& fltset )
    : Executor("Reading FaultSet")
    , fltset_(fltset)
    , curidx_(0)
    , basedir_(ioobj.fullUserExpr(false))
{
}

od_int64 nrDone() const override
{
    return curidx_;
}

od_int64 totalNr() const override
{
    return fltset_.nrFaults();
}

uiString uiNrDoneText() const override
{
    return tr("Faults written");
}

int nextStep() override
{
    const int nrfaults = fltset_.nrFaults();
    if ( curidx_ >= nrfaults )
    {
	fltset_.saveDisplayPars();
	return Finished();
    }

    if ( !File::isDirectory(basedir_) && !File::createDir(basedir_) )
	return ErrorOccurred();

    const EM::FaultID id = fltset_.getFaultID( curidx_ );
    FilePath fp( basedir_, toString(id.asInt()) );
    fp.setExtension( "flt" );
    ConstRefMan<EM::Fault3D> flt = fltset_.getFault3D( id );
    EM::dgbSurfaceWriter wrr( fp.fullPath(), mTranslGroupName(EMFault3D),
			      *flt, EMSurfaceTranslator::getBinarySetting() );
    wrr.execute();
    curidx_++;
    return MoreToDo();
}

	const EM::FaultSet3D&   fltset_;
	BufferString	    basedir_;
	int		    curidx_;

};


Executor* dgbEMFaultSet3DTranslator::reader( EM::FaultSet3D& fltset,
					     const IOObj& ioobj )
{
    return new dGBFaultSet3DReader( ioobj, fltset );
}


Executor* dgbEMFaultSet3DTranslator::writer( const EM::FaultSet3D& fltset,
					     const IOObj& ioobj )
{
    return new dGBFaultSet3DWriter( ioobj, fltset );
}
