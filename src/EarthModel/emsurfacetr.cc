/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          May 2002
 RCS:           $Id: emsurfacetr.cc,v 1.3 2004-07-29 16:52:30 bert Exp $
________________________________________________________________________

-*/

#include "emsurfacetr.h"
#include "emsurfaceio.h"

#include "ascstream.h"
#include "datachar.h"
#include "datainterp.h"
#include "emsurfauxdataio.h"
#include "executor.h"
#include "filegen.h"
#include "ioobj.h"
#include "iopar.h"
#include "iostrm.h"
#include "strmprov.h"
#include "ptrman.h"
#include "survinfo.h"
#include "cubesampling.h"
#include "settings.h"


const char* EMHorizonTranslatorGroup::keyword = "Horizon";

const IOObjContext& EMHorizonTranslatorGroup::ioContext()
{
    static IOObjContext* ctxt = 0;

    if ( !ctxt )
    {
	ctxt = new IOObjContext( 0 );
	ctxt->crlink = false;
	ctxt->newonlevel = 1;
	ctxt->needparent = false;
	ctxt->maychdir = false;
	ctxt->stdseltype = IOObjContext::Surf;
    }

    // Due to static initialisation order ...
    ctxt->trgroup = &theInst();
    return *ctxt;
}


int EMHorizonTranslatorGroup::selector( const char* key )
{
    return defaultSelector( keyword, key );
}


const char* EMFaultTranslatorGroup::keyword = "Fault";

const IOObjContext& EMFaultTranslatorGroup::ioContext()
{
    static IOObjContext* ctxt = 0;

    if ( !ctxt )
    {
	ctxt = new IOObjContext( &theInst() );
	ctxt->crlink = false;
	ctxt->newonlevel = 1;
	ctxt->needparent = false;
	ctxt->maychdir = false;
	ctxt->stdseltype = IOObjContext::Surf;
    }

    return *ctxt;
}


int EMFaultTranslatorGroup::selector( const char* key )
{
    return defaultSelector( keyword, key );
}

// *************************************************************************

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


Executor* EMSurfaceTranslator::writer( const IOObj& ioobj )
{
    setIOObj( &ioobj );
    Executor* ret = getWriter();
    if ( ret )
	fullImplRemove( ioobj );
    return ret;
}


#define mImplStart(fn) \
    if ( !ioobj || strcmp(ioobj->translator(),"dGB") ) return false; \
    mDynamicCastGet(const IOStream*,iostrm,ioobj) \
    if ( !iostrm ) return false; \
 \
    BufferString pathnm = iostrm->dirName(); \
    BufferString basenm = iostrm->fileName(); \
 \
    StreamProvider sp( basenm ); \
    sp.addPathIfNecessary( pathnm ); \
    if ( !sp.fn ) return false;

#define mImplLoopStart \
    if ( gap > 100 ) return true; \
    StreamProvider sp( EM::dgbSurfDataWriter::createHovName(basenm,nr) ); \
    sp.addPathIfNecessary( pathnm );


bool EMSurfaceTranslator::implRemove( const IOObj* ioobj ) const
{
    mImplStart(remove(false));

    int gap = 0;
    for ( int nr=0; ; nr++ )
    {
	mImplLoopStart;
	if ( sp.exists(true) )
	    sp.remove(false);
	else
	    gap++;
    }

    return true;
}


bool EMSurfaceTranslator::implRename( const IOObj* ioobj, const char* newnm,
				      const CallBack* cb ) const
{
    mImplStart(rename(newnm,cb));

    int gap = 0;
    for ( int nr=0; ; nr++ )
    {
	mImplLoopStart;
	StreamProvider spnew( EM::dgbSurfDataWriter::createHovName(newnm,nr) );
	spnew.addPathIfNecessary( pathnm );
	if ( sp.exists(true) )
	    sp.rename( spnew.fileName(), cb );
	else
	    gap++;
    }

    return true;
}


bool EMSurfaceTranslator::implSetReadOnly( const IOObj* ioobj, bool ro ) const
{
    mImplStart(setReadOnly(ro));

    return true;
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


class dgbEMSurfacePosCalc : public EM::SurfPosCalc
{
public:
    Coord	getPos(const RowCol& rc) const
		{
		    return SI().transform(BinID(rc.row,rc.col));
		}
};


class dgbEMSurfaceRowColConverter : public EM::RowColConverter
{
public:
    RowCol		get(const RowCol& rc) const
			{
			    const Coord coord(a11*rc.row+a12*rc.col+a13,
					      a21*rc.row+a22*rc.col+a23 );
			    const BinID bid = SI().transform(coord);
			    return RowCol(bid.inl, bid.crl );
			}

    bool		usePar(const IOPar& par)
			{
			    return par.get( transformxstr, a11, a12, a13 ) &&
				   par.get( transformystr, a21, a22, a23 );
			}

    static const char*	transformxstr;
    static const char*	transformystr;

protected:
    double		a11, a12, a13, a21, a22, a23;
};


const char* dgbEMSurfaceRowColConverter::transformxstr = "X transform";
const char* dgbEMSurfaceRowColConverter::transformystr = "Y transform";


bool dgbEMSurfaceTranslator::prepRead()
{
    if ( reader_ ) delete reader_;
    BufferString unm = group() ? group()->userName() : 0;
    reader_ = new EM::dgbSurfaceReader( *ioobj_, unm, 0 );
    if ( !reader_->isOK() )
    {
	errmsg_ = reader_->message();
	return false;
    }

    int version = 1;
    reader_->pars()->get( EM::dgbSurfaceReader::versionstr, version );
    if ( version==1 )
    {
	dgbEMSurfaceRowColConverter* rctrans = new dgbEMSurfaceRowColConverter;
	if ( !rctrans->usePar(*reader_->pars()) )
	{
	    delete rctrans;
	    errmsg_ = "Cannot parse transform";
	    return false;
	}

	reader_->setRowColConverter( rctrans );
	reader_->setReadFillType( true );
    }

    if ( readOnlyZ() )
	reader_->setSurfPosCalc( new dgbEMSurfacePosCalc );

    for ( int idx=0; idx<reader_->nrSections(); idx++ )
	sd_.sections += new BufferString( reader_->sectionName(idx) );
    
    for ( int idx=0; idx<reader_->nrAuxVals(); idx++ )
	sd_.valnames += new BufferString( reader_->auxDataName(idx) );

    if ( version == 1 )
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

	sd_.dbinfo = reader_->dbInfo();
    }

    return true;
}


void dgbEMSurfaceTranslator::getSels( StepInterval<int>& rrg,
				      StepInterval<int>& crg )
{
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
	reader_->setSurface( surface );
	if ( hasRangeSelection() )
	{
	    StepInterval<int> rrg, crg; getSels( rrg, crg );
	    reader_->setRowInterval( rrg ); reader_->setColInterval( crg );
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
    bool binary = true;
    mSettUse(getYN,"dTect.Surface","Binary format",binary);
    BufferString unm = group() ? group()->userName() : 0;
    EM::dgbSurfaceWriter* res = new EM::dgbSurfaceWriter(ioobj_,unm,
							 *surface,binary);
    res->setWriteOnlyZ( writeOnlyZ() );
    res->pars()->set( EM::dgbSurfaceReader::versionstr, 2 );

    if ( hasRangeSelection() )
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
