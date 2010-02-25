/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2004
________________________________________________________________________

-*/
static const char* rcsID = "$Id: od_process_attrib_em.cc,v 1.64 2010-02-25 13:34:35 cvshelene Exp $";

#include "attribdesc.h"
#include "attribdescid.h"
#include "attribdescset.h"
#include "attribdescsettr.h"
#include "attribengman.h"
#include "attriboutput.h"
#include "attribprocessor.h"
#include "attribsel.h"
#include "attribstorprovider.h"

#include "batchprog.h"
#include "binidvalset.h"
#include "datapointset.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emhorizonutils.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "executor.h"
#include "filegen.h"
#include "filepath.h"
#include "hostdata.h"
#include "initalgo.h"
#include "initattributeengine.h"
#include "initattributes.h"
#include "initearthmodel.h"
#include "initprestackprocessing.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "linesetposinfo.h"
#include "progressmeter.h"
#include "ptrman.h"
#include "seisbuf.h"
#include "seisioobjinfo.h"
#include "seisjobexecprov.h"
#include "seistrc.h"
#include "seiswrite.h"
#include "seis2dline.h"
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


class CallBackMgr : public CallBacker
{
public:
	
    bool		sync2DGeometryCB(CallBacker*);

			CallBackMgr(std::ostream& strm)
			    : CallBacker()
			    , strm_( strm )
			{ 
			    cb_ = mCB(this,CallBackMgr,sync2DGeometryCB);
			    EM::EMM().syncGeomReq.notify( cb_ );
			}
			~CallBackMgr()
			{
			    EM::EMM().syncGeomReq.remove( cb_ );
			}
			
protected:
    CallBack		cb_;
    std::ostream&	strm_;
};


static bool attribSetQuery( std::ostream& strm, const IOPar& iopar,
			    bool stepout, float vnr )
{
    DescSet initialset( false );
    PtrMan<IOPar> attribs = iopar.subselect("Attributes");
    if ( !initialset.usePar( *attribs, vnr ) )
	mErrRet( initialset.errMsg() )

    const BufferString tmpoutstr( IOPar::compKey( sKey::Output, 0 ) );
    const BufferString tmpattribstr( IOPar::compKey( sKey::Attributes, 0 ) );
    const char* res = iopar.find( IOPar::compKey( tmpoutstr.buf(),
						  tmpattribstr.buf() ) );
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
    BufferString lpartstr = IOPar::compKey( sKey::Output, 0 );
    BufferString outstr( IOPar::compKey( lpartstr.buf(), idstr ) );

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
	BufferString basehorstr(
	    IOPar::compKey(sKey::Geometry,LocationOutput::surfidkey()) );
	BufferString hor1str = IOPar::compKey(basehorstr,0);
	if( !getObjectID( iopar, hor1str, true, errmsg, objidstr ) ) 
	    return false;

	MultiID* mid = new MultiID( objidstr.buf() );
	midset += mid;

	BufferString hor2str = IOPar::compKey(basehorstr,1);
	if( !getObjectID( iopar, hor2str, false, errmsg, objidstr ) )
	    return false;

	if ( objidstr.size() )
	{
	    MultiID* mid2 = new MultiID( objidstr.buf() );
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

#define mSetCommState(State) \
    if ( comm ) \
    { \
	comm->setState( MMSockCommunic::State ); \
	if ( !comm->updateState() ) \
	    mRetHostErr( comm->errMsg() ) \
    }


static bool process( std::ostream& strm, Processor* proc, bool useoutwfunc, 
		    const MultiID& outid = 0 , SeisTrcBuf* tbuf = 0 )
{
    bool cont = true;
    bool loading = true;
    int nriter = 0;
    int nrdone = 0;
    SeisTrcWriter* writer( 0 );

    TextStreamProgressMeter progressmeter(strm);
    while ( 1 )
    {
	int res = proc->doStep();

	if ( nriter==0 )
	{
	    strm << "Estimated number of positions to be processed"
		 <<"(regular survey): " << proc->totalNr() << std::endl;
	    strm << "Loading cube data ..." << std::endl;

	    if ( !useoutwfunc && tbuf )
	    {
		PtrMan<IOObj> ioseisout = IOM().get( outid );
		writer = new SeisTrcWriter( ioseisout );
		if ( !tbuf->size() ||!writer->prepareWork(*(tbuf->get(0))) )
		{ 
		    BufferString err = strlen( writer->errMsg() ) ? 
						    writer->errMsg() : 
						    "ERROR: no trace computed";
		    mErrRet( err );
		}
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

	if ( !useoutwfunc && tbuf )
	{
	    if ( !writer->put(*(tbuf->get(0))) )
	    { mErrRet( writer->errMsg() ); }

	    SeisTrc* trc = tbuf->remove(0);
	    delete trc;
	}
	else if ( useoutwfunc && res>= 0 )
	    proc->outputs_[0]->writeTrc();

	nriter++;
    }

    delete writer;
    progressmeter.setFinished();
    mPIDMsg( "Processing done." );

    // It is VERY important workers are destroyed BEFORE the last writeStatus!!!
    mDestroyWorkers
    return true;
}


static HorSampling getHorSamp( IOPar* geompar )
{
    HorSampling hsamp;
    if ( !geompar->get( SurveyInfo::sKeyInlRange(),
			hsamp.start.inl, hsamp.stop.inl )
	 || !geompar->get( SurveyInfo::sKeyCrlRange(),
			   hsamp.start.crl, hsamp.stop.crl ) )
    {
	hsamp.start.inl = 0;
	hsamp.stop.inl = mUdf(int);
	hsamp.start.crl = 0;
	hsamp.stop.crl = mUdf(int);
    }

    return hsamp;
}


bool BatchProgram::go( std::ostream& strm )
{
    Algo::initStdClasses();
    AttributeEngine::initStdClasses();
    Attributes::initStdClasses();
    EarthModel::initStdClasses();
    PreStackProcessing::initStdClasses();

    const float vnr = parversion_.isEmpty() ? 0 : atof( parversion_.buf() );
    if ( cmdLineOpts().size() )
    {
	BufferString opt = *cmdLineOpts()[0];
	bool ismaxstepout = opt == "maxstepout";
	if ( ismaxstepout || opt == "validate" )
	    return attribSetQuery( strm, pars(), ismaxstepout, vnr );
    }

    showHostName( strm );
    Seis2DLineSet::installPreSet( pars(), SeisJobExecProv::sKeyOutputLS(),
				  SeisJobExecProv::sKeyWorkLS() );

    BufferString type;
    pars().get( IOPar::compKey( sKey::Output, sKey::Type ), type );
   
    const bool iscubeoutp = !strcmp( type, Output::tskey() );

    BufferString errmsg;
    MultiID outpid;
    ObjectSet<MultiID> midset;
    if ( !prepare( strm, pars(), 
		   iscubeoutp ? SeisTrcStorOutput::seisidkey() 
		   	      : LocationOutput::surfidkey(),
		   midset, errmsg, iscubeoutp, outpid ) )
	mErrRetNoProc(errmsg);

    PtrMan<IOPar> geompar = pars().subselect(sKey::Geometry);
    HorSampling hsamp;
    BufferString linename;
    if ( iscubeoutp && geompar )
    {
	geompar->get( sKey::LineKey, linename );
	hsamp = getHorSamp( geompar );
    }

    PtrMan<IOPar> mmprocrange =
	pars().subselect( IOPar::compKey(sKey::Output,sKey::Subsel) );
    if ( iscubeoutp && mmprocrange )
    {
	HorSampling mmrange;
	if ( mmrange.usePar( *mmprocrange ) )
	    hsamp.limitTo( mmrange );
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
	sels.rg = hsamp;
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
    DescSet attribset( false );
    PtrMan<IOPar> attribs = pars().subselect( sKey::Attributes );
    if ( !attribset.usePar(*attribs,vnr) )
	mErrRetNoProc( attribset.errMsg() )

    PtrMan<IOPar> output = pars().subselect( IOPar::compKey(sKey::Output,0) );
    if ( !output ) mErrRetNoProc( "No output specified" );
    
    PtrMan<IOPar> attribsiopar = output->subselect( sKey::Attributes );
    if ( !attribsiopar ) mErrRetNoProc( "No output specified" );

    TypeSet<DescID> attribids;
    int nrattribs = 1;
    attribsiopar->get( "MaxNrKeys", nrattribs );
    for ( int idx=0; idx<nrattribs; idx++ )
    {
	int id;
	if ( attribsiopar->get(toString(idx),id) )
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
    pars().get( sKey::Target, newattrnm );
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

	if ( !process( strm, proc, false ) ) return false;
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

	CallBackMgr cbmgr(strm);
	const bool is2d = attribset.is2D();
	BinIDValueSet bivs(2,false);
	TypeSet<DataPointSet::DataRow> startset;
	BufferStringSet valnms;
	valnms.add("z2");
	DataPointSet* dtps = new DataPointSet( startset, valnms, true );
	if ( is2d )
	    HorizonUtils::getWantedPos2D( strm, midset, dtps, hsamp, 
		    			  extraz, linename );
	else
	    HorizonUtils::getWantedPositions( strm, midset, bivs, hsamp,
		    			      extraz, nrinterpsamp, mainhoridx,
					      extrawidth );
	if ( !zboundsset )
	{
	    //fix needed to get homogeneity when using multi-machines processing
	    zboundsset = true;
	    BinIDValueSet* natbounds = is2d ? &dtps->bivSet() : &bivs;
	    zbounds.start = natbounds->valRange(0).start;
	    zbounds.stop = natbounds->valRange(1).stop;
	}

	SeisTrcBuf seisoutp( false );
	Processor* proc =
	    is2d ? aem.create2DVarZOutput( errmsg, pars(), dtps, outval,
		    			   zboundsset ? &zbounds : 0 )
	    	 : aem.createTrcSelOutput( errmsg, bivs, seisoutp, outval,
					   zboundsset ? &zbounds : 0 ); 
	if ( !proc ) mErrRet( errmsg );
	if ( !process( strm, proc, is2d, outpid, &seisoutp ) ) return false;
	
	delete dtps;
    }

    strm << "Successfully saved data." << std::endl;

    deepErase(midset);
    deepUnRef( objects );

    return true;
}


bool CallBackMgr::sync2DGeometryCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );
    mDynamicCastGet( EM::Horizon2D*, h2d, EM::EMM().getObject( emid ) );
    for ( int lidx=0; h2d && lidx<h2d->geometry().nrLines(); lidx++ )
    {
	const int lineid = h2d->geometry().lineID( lidx );
	if ( h2d->geometry().syncBlocked(lineid) )
	    continue;
	const MultiID& lsetid = h2d->geometry().lineSet( lineid );
	const char* linenm = h2d->geometry().lineName( lineid );
	PosInfo::Line2DData ldat;
	PtrMan<IOObj> ioobj = IOM().get( lsetid );
	if ( !ioobj ) return false;
	BufferString fnm = ioobj->fullUserExpr(true);
	Seis2DLineSet lineset( fnm );
	int lineidx = lineset.indexOf( linenm );
	if ( lineidx < 0 )
	{
	    BufferStringSet attribs;
	    SeisIOObjInfo sobjinfo( lsetid );
	    sobjinfo.getAttribNamesForLine( linenm, attribs, true );
	    if ( attribs.isEmpty() ) return false;
	    lineidx = lineset.indexOf( LineKey(linenm,attribs.get(0)) );
	    if ( lineidx < 0 ) return false;
	}

	lineset.getGeometry( lineidx, ldat );
	h2d->geometry().syncLine( lsetid, linenm, ldat );
    }

    return true;
}


