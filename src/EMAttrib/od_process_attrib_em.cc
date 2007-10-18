/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2004
 RCS:           $Id: od_process_attrib_em.cc,v 1.40 2007-10-18 10:58:09 cvsnanne Exp $
________________________________________________________________________

-*/

#include "attribdesc.h"
#include "attribdescid.h"
#include "attribdescset.h"
#include "attribdescsettr.h"
#include "attribengman.h"
#include "attribinit.h"
#include "attriboutput.h"
#include "attribprocessor.h"
#include "attribsel.h"
#include "attribstorprovider.h"

#include "batchprog.h"
#include "binidvalset.h"
#include "emhorizon3d.h"
#include "emhorizonutils.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "executor.h"
#include "filegen.h"
#include "filepath.h"
#include "hostdata.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "progressmeter.h"
#include "ptrman.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "seiswrite.h"
#include "separstr.h"
#include "survinfo.h"
#include "timefun.h"


using namespace Attrib;
using namespace EM;

#define mDestroyWorkers \
	{ delete proc; proc = 0; }


#define mErrRet(s) \
{ \
    strm << (stepout ? "0 0" : "0") << std::endl; \
    std::cerr << s << std::endl; \
    return false; \
}

static bool attribSetQuery( std::ostream& strm, const IOPar& iopar,
			    bool stepout )
{
    DescSet initialset( false, false );
    PtrMan<IOPar> attribs = iopar.subselect("Attributes");
    if ( !initialset.usePar( *attribs ) )
	mErrRet( initialset.errMsg() )

    const char* res = iopar.find( "Output.1.Attributes.0" );
    if ( !res )
	mErrRet( "No target attribute found" )
    DescID outid( atoi( res ), true ); 
    if ( initialset.getDesc(outid) < 0 )
	mErrRet( "Target attribute not present in attribute set" )

    return true;
}


static void showHostName( std::ostream& strm )
{ strm << "Processing on " << HostData::localHostName() << '.' << std::endl; }


static bool getObjectID( const IOPar& iopar, const char* str, bool claimmissing,
			 BufferString& errmsg, BufferString& objidstr )
{
    const char* objid = iopar.find( str );
    if ( !objid && claimmissing )
    {
	errmsg = "No "; errmsg += str; 
	errmsg += " defined in parameter file";
	return false;
    }
    else if ( objid )
    {
	PtrMan<IOObj> ioobj = IOM().get( objid );
	if ( !ioobj )
	{
	    errmsg = "Cannot find object for '"; errmsg += objid; 
	    errmsg += "' ...";
	    return false;
	}

	objidstr = objid;
    }
    else
	objidstr = "";
    
    return true;
}

    
static bool prepare( std::ostream& strm, const IOPar& iopar, const char* idstr,
		     ObjectSet<MultiID>& midset, BufferString& errmsg, 
		     bool iscubeoutp, MultiID& outpid  )
{
    strm << "Preparing processing\n"; strm.flush();
    BufferString outstr( "Output.1." ); outstr += idstr;

    BufferString objidstr;
    if( !getObjectID( iopar, outstr, true, errmsg, objidstr ) ) return false;

    if ( !iscubeoutp )
    {
	MultiID* mid = new MultiID(objidstr.buf());
	midset += mid;
    }
    else
    {
	outpid = objidstr.buf();
	BufferString basehorstr = sKey::Geometry; basehorstr += "."; 
	basehorstr += LocationOutput::surfidkey; 
	BufferString hor1str = basehorstr; hor1str += ".0";
	if( !getObjectID( iopar, hor1str, true, errmsg, objidstr ) ) 
	    return false;

	MultiID* mid = new MultiID(objidstr.buf());
	midset += mid;

	BufferString hor2str = basehorstr; hor2str += ".1";
	if( !getObjectID( iopar, hor2str, false, errmsg, objidstr ) )
	    return false;

	if ( objidstr.size() )
	{
	    MultiID* mid2 = new MultiID(objidstr.buf());
	    midset += mid2;
	}
    }
    return true;
}


#undef mErrRet
#define mErrRet(s) \
    { strm << '\n' << s << '\n' << std::endl; mDestroyWorkers ; return false; }

#define mErrRetNoProc(s) \
    { strm << '\n' << s << '\n' << std::endl; return false; }

#define mPIDMsg(s) { strm << "\n["<< GetPID() <<"]: " << s << std::endl; }

static bool process( std::ostream& strm, Processor* proc, 
		     const MultiID& outid = 0 , SeisTrcBuf* tbuf = 0 )
{
    bool cont = true;
    bool loading = true;
    int nriter = 0;
    int nrdone = 0;
    SeisTrcWriter* writer( 0 );

    ProgressMeter progressmeter(strm);
    while ( 1 )
    {
	int res = proc->doStep();

	if ( nriter==0 )
	{
	    strm << "Estimated number of positions to be processed"
		 <<"(regular survey): " << proc->totalNr() << std::endl;
	    strm << "Loading cube data ..." << std::endl;
	   
	    if ( tbuf )
	    {
		PtrMan<IOObj> ioseisout = IOM().get( outid );
		writer = new SeisTrcWriter( ioseisout );
		if ( !tbuf->size() ||!writer->prepareWork(*(tbuf->get(0))) )
		{ 
		    BufferString err = strlen( writer->errMsg() ) ? 
				       writer->errMsg() : 
				       "ERROR: no trace computed";
		    mErrRet( err ); }
	    }
	}

	if ( res > 0 )
	{
	    if ( loading )
	    {
		loading = false;
		mPIDMsg( "Processing started." );
	    }

	    if ( proc->nrDone()>nrdone )
	    {
		nrdone++;
		++progressmeter;
	    }
	}
	else
	{
	    if ( res == -1 )
		mErrRet( "Cannot reach next position" )
	    break;
	}

	if ( tbuf )
	{
	    if ( !writer->put(*(tbuf->get(0))) )
		{ mErrRet( writer->errMsg() ); }

	    SeisTrc* trc = tbuf->remove(0);
	    delete trc;
	}

	nriter++;
    }

    delete writer;
    progressmeter.finish();
    mPIDMsg( "Processing done." );

    // It is VERY important workers are destroyed BEFORE the last writeStatus!!!
    mDestroyWorkers
    return true;
}


bool BatchProgram::go( std::ostream& strm )
{
    initAttribClasses();
    if ( cmdLineOpts().size() )
    {
	BufferString opt = *cmdLineOpts()[0];
	bool ismaxstepout = opt == "maxstepout";
	if ( ismaxstepout || opt == "validate" )
	    return attribSetQuery( strm, pars(), ismaxstepout );
    }

    showHostName( strm );

    BufferString type;
    pars().get( "Output.1.Type",type );
   
    const bool iscubeoutp = !strcmp( type, Output::tskey );

    BufferString errmsg;
    MultiID outpid;
    ObjectSet<MultiID> midset;
    if ( !prepare( strm, pars(), 
		   iscubeoutp ? SeisTrcStorOutput::seisidkey 
		   	      : LocationOutput::surfidkey,
		   midset, errmsg, iscubeoutp, outpid ) )
	mErrRetNoProc(errmsg);

    PtrMan<IOPar> geompar = pars().subselect(sKey::Geometry);
    HorSampling horsamp;
    if ( iscubeoutp && geompar )
    {
	geompar->get( "In-line range", horsamp.start.inl, horsamp.stop.inl );
	geompar->get( "Cross-line range", horsamp.start.crl, horsamp.stop.crl);
    }

    ObjectSet<EMObject> objects;
    for ( int idx=0; idx<midset.size(); idx++ )
    {
	MultiID* mid = midset[idx];
	strm << "Loading: " << mid->buf() << "\n\n";

	SurfaceIOData sd;
	EM::EMM().getSurfaceData( *mid, sd );
	SurfaceIODataSelection sels( sd );
	sels.selvalues.erase();
	for ( int idx=0; idx<sd.sections.size(); idx++ )
	    sels.selsections += idx;
	sels.rg = horsamp;
	PtrMan<Executor> loader = 
			EMM().objectLoader( *mid, iscubeoutp ? &sels : 0 );
	if ( !loader || !loader->execute(&strm) ) 
	{
	    BufferString errstr = "Cannot load horizon:";
	    errstr += mid->buf();
	    mErrRetNoProc( errstr.buf() );
	}

	EMObject* emobj = EMM().getObject( EMM().getObjectID(*mid) );
	if ( emobj ) emobj->ref();
	objects += emobj;
    }

    StorageProvider::initClass();
    DescSet attribset(false,false);
    PtrMan<IOPar> attribs = pars().subselect( "Attributes" );
    if ( !attribset.usePar(*attribs) )
	mErrRetNoProc( attribset.errMsg() )

    PtrMan<IOPar> output = pars().subselect( "Output.1" );
    if ( !output ) mErrRetNoProc( "No output specified" );
    
    PtrMan<IOPar> attribsiopar = output->subselect("Attributes");
    if ( !attribsiopar ) mErrRetNoProc( "No output specified" );

    TypeSet<DescID> attribids;
    int nrattribs = 1;
    attribsiopar->get( "MaxNrKeys", nrattribs );
    for ( int idx=0; idx<nrattribs; idx++ )
    {
	BufferString key = idx;
	int id;
	if ( attribsiopar->get(key,id) )
	    attribids += DescID(id,true);
    }

    if ( !attribids.size() )
	mErrRetNoProc( "No attributes selected" );

    TypeSet<SelSpec> selspecs;
    BufferStringSet attribrefs;
    for ( int idx=0; idx<attribids.size(); idx++ )
    {
	SelSpec spec( 0, attribids[idx] );
	spec.setRefFromID( attribset );
	selspecs += spec;
	attribrefs.add( spec.userRef() );
    }

    BufferString newattrnm;
    pars().get( "Target value", newattrnm );
    if ( newattrnm != "" )
	attribrefs.get(0) = newattrnm;

    EngineMan aem;
    aem.setAttribSet( &attribset );
    aem.setAttribSpecs( selspecs );

    if ( !iscubeoutp )
    {
	ObjectSet<BinIDValueSet> bivs;
	HorizonUtils::getPositions( strm, *(midset[0]), bivs );
	Processor* proc = aem.createLocationOutput( errmsg, bivs );
	if ( !proc ) mErrRet( errmsg );

	if ( !process(strm,proc) ) return false;
        HorizonUtils::addSurfaceData( *(midset[0]), attribrefs, bivs );
	EMObject* obj = EMM().getObject( EMM().getObjectID(*midset[0]) );
	mDynamicCastGet(Horizon3D*,horizon,obj)
	if ( !horizon ) mErrRet( "Huh" );

	SurfaceIOData sd; sd.use( *horizon );
	SurfaceIODataSelection sels( sd );
	PtrMan<Executor> saver = horizon->auxdata.auxDataSaver( -1, false );
	if ( !saver || !saver->execute(&strm) )
	    mErrRet( "Cannot save data" );
    }
    else
    {
	float outval;
	geompar->get( "Outside Value", outval );

	Interval<float> extraz;
	geompar->get( "ExtraZInterval", extraz.start, extraz.stop );
	extraz.scale(1/SI().zFactor());

	int nrinterpsamp = 0;
	geompar->get( "Interpolation Stepout", nrinterpsamp );

	int mainhoridx = 1;
	geompar->get( "Leading Horizon", mainhoridx );

	float extrawidth = 0;
	geompar->get( "Artificial Width", extrawidth );
	extrawidth /= SI().zFactor();

	bool zboundsset = false;
	Interval<float> zbounds;
	if ( geompar->get("Z Boundaries",zbounds.start,zbounds.stop) )
	{
	    zboundsset = true;
	    zbounds.scale( 1/SI().zFactor() );
	}

	BinIDValueSet bivs(2,false);
	HorizonUtils::getWantedPositions( strm, midset, bivs, horsamp, extraz,
					  nrinterpsamp, mainhoridx, extrawidth);
	SeisTrcBuf seisoutp( false );
	Processor* proc = 
	    aem.createTrcSelOutput( errmsg, bivs, seisoutp, outval,
		    		    zboundsset ? &zbounds : 0 );
	if ( !proc ) mErrRet( errmsg );
	if ( !process( strm, proc, outpid, &seisoutp ) ) return false;
    }

    strm << "Successfully saved data." << std::endl;

    deepErase(midset);
    deepUnRef( objects );

    return true;
}
