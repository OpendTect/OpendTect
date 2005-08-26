/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2004
 RCS:           $Id: od_process_attrib_em.cc,v 1.11 2005-08-26 18:19:27 cvsbert Exp $
________________________________________________________________________

-*/

#include "attribstorprovider.h"
#include "attribdescset.h"
#include "attribdesc.h"
#include "attribdescid.h"
#include "attribdescsettr.h"
#include "attribprocessor.h"
#include "attribengman.h"
#include "attriboutput.h"
#include "batchprog.h"
#include "iopar.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "progressmeter.h"
#include "hostdata.h"
#include "separstr.h"
#include "timefun.h"
#include "filegen.h"
#include "filepath.h"

#include "emmanager.h"
#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "parametricsurface.h"
#include "executor.h"
#include "survinfo.h"

#include "attribsel.h"
#include "attribfactory.h"
#include "attrfact.h"


#define mDestroyWorkers \
	{ delete execgr; execgr = 0; }


#define mErrRet(s) \
{ \
    strm << (stepout ? "0 0" : "0") << std::endl; \
    std::cerr << s << std::endl; \
    return false; \
}

static bool attribSetQuery( std::ostream& strm, const IOPar& iopar,
			    bool stepout )
{
    Attrib::DescSet initialset;
    PtrMan<IOPar> attribs = iopar.subselect("Attributes");
    if ( !initialset.usePar( *attribs ) )
	mErrRet( initialset.errMsg() )

    const char* res = iopar.find( "Output.1.Attributes.0" );
    if ( !res )
	mErrRet( "No target attribute found" )
    Attrib::DescID outid( atoi( res ), true ); 
    if ( initialset.getDesc(outid) < 0 )
	mErrRet( "Target attribute not present in attribute set" )

    return true;
}


static void getPositions( std::ostream& strm, const MultiID& id,
			  ObjectSet<BinIDValueSet>& data )
{
    EM::EMManager& em = EM::EMM();
    EM::ObjectID objid = em.multiID2ObjectID(id);
    EM::EMObject* obj = em.getObject( objid );
    mDynamicCastGet(EM::Surface*,surface,obj)
    if ( !surface ) return;

    strm << "\nFetching surface positions ...\n" ;
    ProgressMeter pm( strm );
    deepErase( data );
    const int nrsect = surface->geometry.nrSections();
    for ( int sectionidx=0; sectionidx<nrsect; sectionidx++ )
    {
	const EM::SectionID sectionid = 
	    			surface->geometry.sectionID( sectionidx );
	const Geometry::ParametricSurface* psurf = 
	    			surface->geometry.getSurface(sectionid);

	BinIDValueSet& res = *new BinIDValueSet( 1, false );
	data += &res;

	const int nrnodes = psurf->nrKnots();
	for ( int idy=0; idy<nrnodes; idy++ )
	{
	    const Coord3 crd = psurf->getKnot( psurf->getKnotRowCol(idy) );
	    const BinID bid = SI().transform(crd);
	    res.add( bid, crd.z );
	    ++pm;
	}
    }

    pm.finish();
    strm << "Done!" << std::endl;
}


static void addSurfaceData( const MultiID& id, const BufferStringSet& attrnms,
			    const ObjectSet<BinIDValueSet>& data )
{
    EM::EMManager& em = EM::EMM();
    EM::ObjectID objid = em.multiID2ObjectID(id);
    EM::EMObject* obj = em.getObject( objid );
    mDynamicCastGet(EM::Surface*,surface,obj)
    if ( !surface ) return;

    surface->auxdata.removeAll();
    for ( int idx=0; idx<attrnms.size(); idx++ )
	surface->auxdata.addAuxData( attrnms.get(idx) );

    for ( int sectionidx=0; sectionidx<data.size(); sectionidx++ )
    {
	const EM::SectionID sectionid = 
	    			surface->geometry.sectionID( sectionidx );
	BinIDValueSet& bivs = *data[sectionidx];

	EM::PosID posid( objid, sectionid );
	BinIDValueSet::Pos pos;
	BinID bid; TypeSet<float> vals;
	while ( bivs.next(pos) )
	{
	    bivs.get( pos, bid, vals );
	    const EM::SubID subid = 
		surface->geometry.rowCol2SubID( RowCol(bid.inl,bid.crl) );
	    posid.setSubID( subid );
	    for ( int validx=1; validx<vals.size(); validx++ )
		surface->auxdata.setAuxDataVal( validx-1, posid, vals[validx] );
	}
    }
}


static void showHostName( std::ostream& strm )
{ strm << "Processing on " << HostData::localHostName() << '.' << std::endl; }


static bool prepare( std::ostream& strm, const IOPar& iopar, const char* idstr,
		     MultiID& mid, BufferString& errmsg )
{
    strm << "Preparing processing\n"; strm.flush();
    BufferString outstr( "Output.1." ); outstr += idstr;
    const char* objid = iopar.find( outstr );
    if ( !objid )
    {
	errmsg = "No "; errmsg += outstr; 
	errmsg += " defined in parameter file";
	return false;
    }
    else
    {
	PtrMan<IOObj> ioobj = IOM().get( objid );
	if ( !ioobj )
	{
	    errmsg = "Cannot find object for '"; errmsg += objid; 
	    errmsg += "' ...";
	    return false;
	}
    }

    mid = objid;
    return true;
}


#undef mErrRet
#define mErrRet(s) \
    { strm << '\n' << s << '\n' << std::endl; mDestroyWorkers ; return false; }

#define mPIDMsg(s) { strm << "\n["<< GetPID() <<"]: " << s << std::endl; }

bool BatchProgram::go( std::ostream& strm )
{
    Attrib::initAttribClasses();
    if ( cmdLineOpts().size() )
    {
	BufferString opt = *cmdLineOpts()[0];
	bool ismaxstepout = opt == "maxstepout";
	if ( ismaxstepout || opt == "validate" )
	    return attribSetQuery( strm, pars(), ismaxstepout );
    }

    showHostName( strm );

    ExecutorGroup* execgr = 0;

    BufferString errmsg;
    MultiID mid;
    if ( !prepare( strm, pars(), "Surface ID", mid, errmsg ) )
	mErrRet( errmsg );

    strm << "Loading: " << mid.buf() << "\n\n";
    PtrMan<Executor> loader = EM::EMM().objectLoader( mid, 0 );
    if ( !loader || !loader->execute(&strm) ) mErrRet( "Cannot load surface" );

    Attrib::StorageProvider::initClass();
    Attrib::DescSet attribset;
    PtrMan<IOPar> attribs = pars().subselect( "Attributes" );
    if ( !attribset.usePar(*attribs) )
	mErrRet( attribset.errMsg() )

    PtrMan<IOPar> output = pars().subselect( "Output.1" );
    if ( !output ) mErrRet( "No output specified" );
    
    PtrMan<IOPar> attribsiopar = output->subselect("Attributes");
    if ( !attribsiopar ) mErrRet( "No output specified" );

    TypeSet<Attrib::DescID> attribids;
    int nrattribs = 1;
    attribsiopar->get( "MaxNrKeys", nrattribs );
    for ( int idx=0; idx<nrattribs; idx++ )
    {
	BufferString key = idx;
	int id;
	if ( attribsiopar->get(key,id) )
	    attribids += Attrib::DescID(id,true);
    }

    if ( !attribids.size() )
	mErrRet( "No attributes selected" );

    TypeSet<Attrib::SelSpec> selspecs;
    BufferStringSet attribrefs;
    for ( int idx=0; idx<attribids.size(); idx++ )
    {
	Attrib::SelSpec spec( 0, attribids[idx] );
	spec.setRefFromID( attribset );
	selspecs += spec;
	attribrefs.add( spec.userRef() );
    }

    // TODO: make a targetvalue for each output
    BufferString newattrnm;
    pars().get( "Target value", newattrnm );
    if ( newattrnm != "" )
	attribrefs.get(0) = newattrnm;

    Attrib::EngineMan aem;
    aem.setAttribSet( &attribset );
    aem.setAttribSpecs( selspecs );

    ObjectSet<BinIDValueSet> bivs;
    getPositions( strm, mid, bivs );
    execgr = aem.locationOutputCreator( errmsg, bivs );

    bool cont = true;
    bool loading = true;
    int nriter = 0;

    ProgressMeter progressmeter(strm);
    while ( 1 )
    {
	int res = execgr->doStep();

	if ( nriter==0 )
	{
	    strm << "Estimated number of positions to be processed"
		 <<"(regular survey): " << execgr->totalNr() << std::endl;
	    strm << "Loading cube data ..." << std::endl;
	}

	if ( res > 0 )
	{
	    if ( loading )
	    {
		loading = false;
		mPIDMsg( "Processing started." );
	    }
	    ++progressmeter;
	}
	else
	{
	    if ( res == -1 )
		mErrRet( "Cannot reach next position" )
	    break;
	}
    }
    nriter++;

    progressmeter.finish();
    mPIDMsg( "Processing done." );

    // It is VERY important workers are destroyed BEFORE the last writeStatus!!!
    mDestroyWorkers

    addSurfaceData( mid, attribrefs, bivs );
    EM::EMObject* obj = EM::EMM().getObject( EM::EMM().multiID2ObjectID(mid) );
    mDynamicCastGet(EM::Surface*,surface,obj)
    if ( !surface ) mErrRet( "Huh" );

    EM::SurfaceIOData sd; sd.use( *surface );
    EM::SurfaceIODataSelection sels( sd );
    PtrMan<Executor> saver = surface->auxdata.auxDataSaver( -1, -1 );
    if ( !saver || !saver->execute(&strm) )
	mErrRet( "Cannot save data" );

    strm << "Successfully saved data." << std::endl;

    return true;
}
