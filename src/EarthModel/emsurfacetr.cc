/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "emsurfacetr.h"

#include "emsurface.h"
#include "emsurfaceio.h"
#include "emsurfauxdataio.h"
#include "executor.h"
#include "filepath.h"
#include "iostrm.h"
#include "strmprov.h"
#include "survinfo.h"
#include "settings.h"


const char* EMHorizon3DTranslatorGroup::keyword()   { return "Horizon"; }

mDefSimpleTranslatorSelector(EMHorizon3D,keyword())

const IOObjContext& EMHorizon3DTranslatorGroup::ioContext()
{
    static IOObjContext* ctxt = 0;
    if ( !ctxt )
    {
	ctxt = new IOObjContext( 0 );
	ctxt->stdseltype = IOObjContext::Surf;
	ctxt->toselect.allowtransls_ = mDGBKey;
    }
    ctxt->trgroup = &theInst();
    return *ctxt;
}


const char* EMHorizon2DTranslatorGroup::keyword()	{ return "2D Horizon"; }

mDefSimpleTranslatorSelector(EMHorizon2D,keyword())

const IOObjContext& EMHorizon2DTranslatorGroup::ioContext()
{
    static IOObjContext* ctxt = 0;
    if ( !ctxt )
    {
	ctxt = new IOObjContext( 0 );
	ctxt->stdseltype = IOObjContext::Surf;
	ctxt->toselect.allowtransls_ = mDGBKey;
    }
    ctxt->trgroup = &theInst();
    return *ctxt;
}


const char* EMAnyHorizonTranslatorGroup::keyword()	{ return "Any Horizon"; }
mDefSimpleTranslatorioContextWithExtra(EMAnyHorizon,Surf,
				       ctxt->toselect.allowtransls_=mDGBKey)

int EMAnyHorizonTranslatorGroup::selector( const char* s )
{ 
    int retval3d = defaultSelector( EMHorizon3DTranslatorGroup::keyword(), s );
    int retval2d = defaultSelector( EMHorizon2DTranslatorGroup::keyword(), s );
    return retval3d ? retval3d : retval2d;
}


const char* EMFault3DTranslatorGroup::keyword()		{ return "Fault"; }
mDefSimpleTranslatorSelector(EMFault3D,keyword())
mDefSimpleTranslatorioContext(EMFault3D,Surf)

const char* EMFaultStickSetTranslatorGroup::keyword()	{ return "FaultStickSet"; }
mDefSimpleTranslatorSelector(EMFaultStickSet,keyword())
mDefSimpleTranslatorioContext(EMFaultStickSet,Surf)


EMSurfaceTranslator::~EMSurfaceTranslator()
{
    delete ioobj_;
}


void EMSurfaceTranslator::init( const EM::Surface* surf, const IOObj* ioobj )
{
    surface = const_cast<EM::Surface*>(surf);
    setIOObj( ioobj );
}


void EMSurfaceTranslator::setIOObj( const IOObj* ioobj )
{
    delete ioobj_;
    ioobj_ = ioobj ? ioobj->clone() : 0;
}


bool EMSurfaceTranslator::startRead( const IOObj& ioobj )
{
    init( 0, &ioobj );
    if ( !prepRead() )
	return false;
    sels_.setDefault();
    return true;
}


bool EMSurfaceTranslator::startWrite( const EM::Surface& surf )
{
    init( &surf, 0 );
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
	fullImplRemove( ioobj );
    return ret;
}


#define mImplStart(fn) \
    if ( !ioobj || strcmp(ioobj->translator(),"dGB") ) return false; \
    const BufferString basefnm( ioobj->fullUserExpr(true) ); \
    StreamProvider sp( basefnm.buf() ); \
    FilePath basefp( basefnm ); \
    const BufferString pathnm( basefp.pathOnly() ); \
    bool res = sp.fn;

#define mImplLoopStart \
    if ( gap > 100 ) break; \
    StreamProvider loopsp( EM::dgbSurfDataWriter::createHovName(basefnm.buf(),nr).buf() )


bool EMSurfaceTranslator::implRemove( const IOObj* ioobj ) const
{
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

    return res;
}


bool EMSurfaceTranslator::implRename( const IOObj* ioobj, const char* newnm,
				      const CallBack* cb ) const
{
    mImplStart(rename(newnm,cb));

    int gap = 0;
    for ( int nr=0; ; nr++ )
    {
	mImplLoopStart;
	StreamProvider spnew(
		EM::dgbSurfDataWriter::createHovName(newnm,nr).buf() );
	spnew.addPathIfNecessary( pathnm.buf() );
	if ( loopsp.exists(true) )
	    loopsp.rename( spnew.fileName(), cb );
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
	oldsp.rename( newfp.fullPath().buf(), cb ); }

    mRename( getSetupFileName )
    mRename( getParFileName )
    
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
    if ( reader_ ) delete reader_;
    BufferString unm = group() ? group()->userName() : 0;
    reader_ = new EM::dgbSurfaceReader( *ioobj_, unm.buf() );
    if ( !reader_->isOK() )
    {
	errmsg_ = reader_->message();
	return false;
    }

    for ( int idx=0; idx<reader_->nrSections(); idx++ )
	sd_.sections.add( reader_->sectionName(idx) );
    
    for ( int idx=0; idx<reader_->nrAuxVals(); idx++ )
    {
	sd_.valnames.add( reader_->auxDataName(idx) );
	sd_.valshifts_ += reader_->auxDataShift(idx);
    }

    for ( int idx=0; idx<reader_->nrLines(); idx++ )
    {
	sd_.linenames.add( reader_->lineName(idx) );
	sd_.linesets.add( reader_->lineSet(idx) );
	StepInterval<int> trcrange = reader_->lineTrcRanges(idx);
	if ( !mIsUdf(trcrange.start) && !mIsUdf(trcrange.stop) )
	    sd_.trcranges += reader_->lineTrcRanges(idx);
    }

    const int version = reader_->version();
    if ( version==1 )
    {
	sd_.rg.start = SI().sampling(false).hrg.start;
	sd_.rg.stop = SI().sampling(false).hrg.stop;
	sd_.rg.step = SI().sampling(false).hrg.step;
    }
    else
    {
	sd_.rg.start.inl = reader_->rowInterval().start;
	sd_.rg.stop.inl = reader_->rowInterval().stop;
	sd_.rg.step.inl = reader_->rowInterval().step;
	sd_.rg.start.crl = reader_->colInterval().start;
	sd_.rg.stop.crl = reader_->colInterval().stop;
	sd_.rg.step.crl = reader_->colInterval().step;
	sd_.zrg.start = reader_->zInterval().start;
	sd_.zrg.stop = reader_->zInterval().stop;

	sd_.dbinfo = reader_->dbInfo();
    }

    reader_->setReadOnlyZ(  readOnlyZ() );

    return true;
}


void dgbEMSurfaceTranslator::getSels( StepInterval<int>& rrg,
				      StepInterval<int>& crg )
{
    if ( sels_.rg.isEmpty() )
	sels_.rg = sd_.rg;

    rrg.start = sels_.rg.start.inl; rrg.stop = sels_.rg.stop.inl;
    rrg.step = sels_.rg.step.inl;
    crg.start = sels_.rg.start.crl; crg.stop = sels_.rg.stop.crl;
    crg.step = sels_.rg.step.crl;
}


Executor* dgbEMSurfaceTranslator::reader( EM::Surface& surf )
{
    surface = &surf;
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
	TypeSet<EM::SectionID> sectionids;
	for ( int idx=0; idx<sels_.selsections.size(); idx++ )
	    sectionids += reader_->sectionID( sels_.selsections[idx] );
	
	reader_->selSections( sectionids );
	reader_->selAuxData( sels_.selvalues );
    }

    reader_ = 0;
    return res;
}

Executor* dgbEMSurfaceTranslator::getWriter()
{
    BufferString unm = group() ? group()->userName() : 0;
    EM::dgbSurfaceWriter* res =
	new EM::dgbSurfaceWriter(ioobj_,unm.buf(), *surface,getBinarySetting());
    res->setWriteOnlyZ( writeOnlyZ() );

    if ( hasRangeSelection() && !sels_.rg.isEmpty() )
    {
	StepInterval<int> rrg, crg; getSels( rrg, crg );
	res->setRowInterval( rrg ); res->setColInterval( crg );
    }
    TypeSet<EM::SectionID> sectionids;
    for ( int idx=0; idx<sels_.selsections.size(); idx++ )
	sectionids += res->sectionID( sels_.selsections[idx] );
    
    res->selSections( sectionids );
    res->selAuxData( sels_.selvalues );

    return res;
}
