/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2004
________________________________________________________________________

-*/

#include "attribdesc.h"
#include "attribdescid.h"
#include "attribdescset.h"
#include "attribdescsettr.h"
#include "attribengman.h"
#include "attriboutput.h"
#include "attribprocessor.h"
#include "attribsel.h"
#include "attribstorprovider.h"

#include "array2dinterpol.h"
#include "arraynd.h"
#include "batchprog.h"
#include "datapointset.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emhorizonutils.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "executor.h"
#include "ioman.h"
#include "keystrs.h"
#include "linesetposinfo.h"
#include "posinfo2d.h"
#include "posprovider.h"
#include "progressmeter.h"
#include "seisbuf.h"
#include "seisioobjinfo.h"
#include "seisjobexecprov.h"
#include "seistrc.h"
#include "seiswrite.h"
#include "separstr.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "moddepmgr.h"
#include "commandlineparser.h"

using namespace Attrib;
using namespace EM;

#define mDestroyWorkers \
	{ deleteAndZeroPtr( proc ); }


#define mErrRet(s) \
{ \
    strm << (stepout ? "0 0\n" : "0\n"); \
    ErrMsg( s ); return false; \
}


static bool attribSetQuery( od_ostream& strm, const IOPar& iopar, bool stepout )
{
    DescSet initialset( false );
    PtrMan<IOPar> attribs = iopar.subselect("Attributes");
    if ( !initialset.usePar(*attribs) )
	mErrRet( initialset.errMsg().getFullString() )

    const BufferString tmpoutstr( IOPar::compKey( sKey::Output(), 0 ) );
    const BufferString tmpattribstr( IOPar::compKey( sKey::Attributes(), 0 ) );
    const char* res = iopar.find( IOPar::compKey( tmpoutstr.buf(),
						  tmpattribstr.buf() ) );
    if ( !res )
	mErrRet( "No target attribute found" )
    DescID outid( toInt( res ), false );
    if ( !initialset.getDesc(outid) )
	mErrRet( "Target attribute not present in attribute set" )

    return true;
}


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


static bool prepare( od_ostream& strm, const IOPar& iopar, const char* idstr,
		     ObjectSet<MultiID>& midset, BufferString& errmsg,
		     bool iscubeoutp, MultiID& outpid  )
{
    strm << "Preparing processing" << od_endl;
    BufferString lpartstr = IOPar::compKey( sKey::Output(), 0 );
    BufferString outstr( IOPar::compKey( lpartstr.buf(), idstr ) );

    BufferString objidstr;
    if( !getObjectID( iopar, outstr, true, errmsg, objidstr ) ) return false;

    if ( !iscubeoutp )
    {
	MultiID* mid = new MultiID(objidstr.buf());
	midset += mid;
	BufferString newattrnm;
	iopar.get( sKey::Target(), newattrnm );
	strm << "Calculating Horizon Data '" << newattrnm << "'." << od_endl;
    }
    else
    {
	outpid.fromString( objidstr.buf() );
	PtrMan<IOObj> ioobj = IOM().get( outpid ); //check already done
	if ( !ioobj ) return false;

	strm << "Calculating '" << ioobj->name() << "'." << od_endl;
	BufferString basehorstr(
	    IOPar::compKey(sKey::Geometry(),LocationOutput::surfidkey()) );
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
    { strm << '\n' << s << "\n\n"; mDestroyWorkers ; return false; }

#define mErrRetNoProc(s) \
    { strm << '\n' << s << "\n\n"; return false; }

#define mPIDMsg(s) { strm << "\n["<< GetPID() <<"]: " << s << '\n'; }


static bool process( od_ostream& strm, Processor*& proc, bool useoutwfunc,
		     const BufferStringSet& refs,
		     const MultiID outid=MultiID::udf(),
		     SeisTrcBuf* tbuf=nullptr )
{
    if ( !proc )
	return false;

    bool loading = true;
    int nriter = 0;
    int nrdone = 0;
    PtrMan<SeisTrcWriter> writer;

    TextStreamProgressMeter progressmeter(strm);
    bool needswriterinit = !useoutwfunc && tbuf;
    while ( 1 )
    {
	int res = proc->doStep();

	if ( nriter==0 )
	{
	    strm << "Estimated number of positions to be processed"
		 <<"(regular survey): " << proc->totalNr() << od_newline;
	    strm << "Loading cube data ..." << od_newline;
	}

	if ( needswriterinit )
	{
	    if ( tbuf && !tbuf->isEmpty() )
	    {
		PtrMan<IOObj> ioseisout = IOM().get( outid );
		if ( !ioseisout )
		    mErrRet("Cannot find database entry")

		writer = new SeisTrcWriter( *ioseisout );
		writer->setComponentNames( refs );
		if ( !writer->prepareWork(*(tbuf->get(0))) )
		{
		    BufferString err = !writer->errMsg().isEmpty()
			?  writer->errMsg().getFullString().buf()
			: "ERROR: Cannot write traces";
		    mErrRet( err );
		}

		needswriterinit = false;
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

	if ( writer && tbuf->get(0) )
	{
	    if ( !writer->put(*(tbuf->get(0))) )
	    { mErrRet( writer->errMsg().getFullString() ); }

	    delete tbuf->remove(0);
	}
	else if ( useoutwfunc && res>= 0 )
	    proc->outputs_[0]->writeTrc();

	nriter++;
    }

    if ( nriter && useoutwfunc )
	proc->outputs_[0]->finishWrite();

    progressmeter.setFinished();
    mPIDMsg( "Processing done." );

    // It is VERY important workers are destroyed BEFORE the last writeStatus!!!
    mDestroyWorkers
    return true;
}


static TrcKeySampling getHorSamp( IOPar& geompar )
{
    TrcKeySampling hsamp;
    if ( !geompar.get( SurveyInfo::sKeyInlRange(),
			hsamp.start_.inl(), hsamp.stop_.inl() )
	 || !geompar.get( SurveyInfo::sKeyCrlRange(),
			   hsamp.start_.crl(), hsamp.stop_.crl() ) )
    {
	hsamp.start_.inl() = 0;
	hsamp.stop_.inl() = mUdf(int);
	hsamp.start_.crl() = 0;
	hsamp.stop_.crl() = mUdf(int);
    }

    return hsamp;
}


static void interpolate( EM::Horizon3D* horizon,
			 const BufferStringSet& attribrefs, IOPar& par,
			 od_ostream& strm )
{
    PtrMan<IOPar> gridpar = par.subselect( "Grid" );
    if ( !gridpar )
	return;

    BufferString gridmethodnm;
    gridpar->get( sKey::Name(), gridmethodnm );
    PtrMan<Array2DInterpol> arr2dint =
	Array2DInterpol::factory().create( gridmethodnm );
    if ( !arr2dint )
	return;

    arr2dint->usePar( *gridpar );
    for ( int idx=0; idx<attribrefs.size(); idx++ )
    {
	const int dataid =
	    horizon->auxdata.auxDataIndex( attribrefs.get(idx).buf() );
	PtrMan< Array2D<float> > attrarr =
	    horizon->auxdata.createArray2D( dataid, horizon->sectionID(0) );
	strm << "Gridding " << attribrefs.get(idx).buf() << "\n";

	TextStreamProgressMeter runner( strm );
	( (Task*)arr2dint )->setProgressMeter( &runner );
	arr2dint->setArray( *attrarr );
	arr2dint->execute();
	runner.setFinished();
	horizon->auxdata.setArray2D( dataid, horizon->sectionID(0),
				     *attrarr );
    }
}


#define mSetEngineMan() \
    EngineMan aem; \
    DescSet localattribset( attribset ); \
    aem.setAttribSet( &localattribset ); \
    aem.setAttribSpecs( selspecs ); \

mLoad2Modules("Attributes","PreStackProcessing")

bool BatchProgram::doWork( od_ostream& strm )
{
    if ( clParser().nrArgs() )
    {
	const bool ismaxstepout = clParser().isPresent( "maxstepout" );
	if ( ismaxstepout || clParser().isPresent( "validate" ) )
	    return attribSetQuery( strm, pars(), ismaxstepout );
    }

    BufferString type;
    pars().get( IOPar::compKey( sKey::Output(), sKey::Type() ), type );

    const bool iscubeoutp = type == Output::tskey();

    BufferString errmsg;
    MultiID outpid;
    ObjectSet<MultiID> midset;
    if ( !prepare( strm, pars(),
		   iscubeoutp ? SeisTrcStorOutput::seisidkey()
			      : LocationOutput::surfidkey(),
		   midset, errmsg, iscubeoutp, outpid ) )
	mErrRetNoProc(errmsg);

    PtrMan<IOPar> geompar = pars().subselect(sKey::Geometry());
    TrcKeySampling hsamp;
    if ( iscubeoutp && geompar )
	hsamp = getHorSamp( *geompar );

    PtrMan<IOPar> mmprocrange =
	pars().subselect( IOPar::compKey(sKey::Output(),sKey::Subsel()) );
    if ( iscubeoutp && mmprocrange )
    {
	TrcKeySampling mmrange;
	if ( mmrange.usePar( *mmprocrange ) )
	    hsamp.limitTo( mmrange );
    }

    Interval<float> zbounds4mmproc;
    ObjectSet<EMObject> objects;
    for ( int idx=0; idx<midset.size(); idx++ )
    {
	MultiID* mid = midset[idx];
	strm << "Loading: " << mid->toString() << "\n\n";

	SurfaceIOData sd;
	uiString uierr;
	if ( !EM::EMM().getSurfaceData(*mid,sd,uierr) )
	{
	    BufferString errstr( "Cannot load horizon ", mid->toString(), ": ");
	    errstr += uierr.getFullString();
	    mErrRetNoProc( errstr.buf() );
	}

	SurfaceIODataSelection sels( sd );
	sels.selvalues.erase();
	for ( int ids=0; ids<sd.sections.size(); ids++ )
	    sels.selsections += ids;
	sels.rg = hsamp;
	PtrMan<Executor> loader =
		EMM().objectLoader( *mid, iscubeoutp ? &sels : nullptr );
	if ( !loader || !loader->go(strm) )
	{
	    BufferString errstr = "Cannot load horizon: ";
	    errstr += mid->toString();
	    mErrRetNoProc( errstr.buf() );
	}

	if ( mIsUdf( sd.zrg.start ) )
	    zbounds4mmproc = SI().zRange( true );
	else
	{
	    if ( idx )
	    {
		zbounds4mmproc.start = sd.zrg.start < zbounds4mmproc.start ?
					sd.zrg.start : zbounds4mmproc.start;
		zbounds4mmproc.stop = sd.zrg.stop > zbounds4mmproc.stop ?
					sd.zrg.stop : zbounds4mmproc.stop;
	    }
	    else
		zbounds4mmproc = sd.zrg;
	}

	EMObject* emobj = EMM().getObject( EMM().getObjectID(*mid) );
	if ( emobj ) emobj->ref();
	objects += emobj;
    }

    DescSet attribset( false );
    PtrMan<IOPar> attribs = pars().subselect( sKey::Attributes() );
    if ( !attribs ) mErrRetNoProc( "No Attributes specified" );

    if ( !attribset.usePar(*attribs) )
	mErrRetNoProc( attribset.errMsg().getFullString() )

    PtrMan<IOPar> output = pars().subselect( IOPar::compKey(sKey::Output(),0) );
    if ( !output ) mErrRetNoProc( "No output specified" );

    PtrMan<IOPar> attribsiopar = output->subselect( sKey::Attributes() );
    if ( !attribsiopar ) mErrRetNoProc( "No output specified" );

    TypeSet<DescID> attribids;
    int nrattribs = 1;
    attribsiopar->get( "MaxNrKeys", nrattribs );
    for ( int idx=0; idx<nrattribs; idx++ )
    {
	int id;
	if ( attribsiopar->get(::toString(idx),id) )
	    attribids += DescID(id,false);
    }

    if ( !attribids.size() )
	mErrRetNoProc( "No attributes selected" );

    TypeSet<SelSpec> selspecs;
    BufferStringSet attribrefs;
    for ( int idx=0; idx<attribids.size(); idx++ )
    {
	SelSpec spec( nullptr, attribids[idx] );
	spec.setRefFromID( attribset );
	selspecs += spec;
	const LineKey lk( spec.userRef() );
	attribrefs.add( !lk.attrName().isEmpty() ? lk.attrName().buf()
						 : lk.buf() );
    }

    BufferString newattrnm;
    pars().get( sKey::Target(), newattrnm );
    if ( attribrefs.size()==1 && newattrnm != "" )
	attribrefs.get(0) = newattrnm;

    if ( !iscubeoutp )
    {
	ObjectSet<BinIDValueSet> bivs;
	HorizonUtils::getPositions( strm, *(midset[0]), bivs );
	uiString uierrmsg;
	mSetEngineMan()
	Processor* proc = aem.createLocationOutput( uierrmsg, bivs );
	if ( !proc ) mErrRet( uierrmsg.getFullString() );

	if ( !process(strm,proc,false,attribrefs) )
	    return false;

	HorizonUtils::addSurfaceData( *(midset[0]), attribrefs, bivs );
	EMObject* obj = EMM().getObject( EMM().getObjectID(*midset[0]) );
	mDynamicCastGet(Horizon3D*,horizon,obj)
	if ( !horizon ) mErrRet( "Huh" );

	interpolate( horizon, attribrefs, pars(), strm );

	SurfaceIOData sd; sd.use( *horizon );
	SurfaceIODataSelection sels( sd );
	PtrMan<Executor> saver = horizon->auxdata.auxDataSaver( -1, true );
	if ( !saver || !saver->go(strm) )
	    mErrRet( "Cannot save data" );
    }
    else if ( geompar )
    {
	float outval;
	geompar->get( "Outside Value", outval );

	Interval<float> extraz;
	geompar->get( "ExtraZInterval", extraz.start, extraz.stop );
	extraz.scale(1.f/SI().zDomain().userFactor());

	int nrinterpsamp = 0;
	geompar->get( "Interpolation Stepout", nrinterpsamp );

	int mainhoridx = 1;
	geompar->get( "Leading Horizon", mainhoridx );

	float extrawidth = 0;
	geompar->get( "Artificial Width", extrawidth );
	extrawidth /= SI().zDomain().userFactor();

	bool zboundsset = false;
	Interval<float> zbounds;
	if ( geompar->get("Z Boundaries",zbounds.start,zbounds.stop) )
	{
	    zboundsset = true;
	    zbounds.scale( 1.f/SI().zDomain().userFactor() );
	}

	if ( !zboundsset && mmprocrange )
	{
	    //fix needed to get homogeneity when using multi-machines processing
	    zboundsset = true;
	    zbounds = zbounds4mmproc==SI().zRange(1) ? zbounds4mmproc
						     : zbounds4mmproc + extraz;
	}

	const bool is2d = attribset.is2D();
	if ( is2d )
	{
	    int lidx = 0;
	    while ( true )
	    {
		if ( !mmprocrange )
		    mErrRetNoProc( "Invalid Output Range" );

		PtrMan<IOPar> linepar = mmprocrange->subselect(
					IOPar::compKey(sKey::Line(),lidx++) );
		if ( !linepar )
		    break;

		Pos::GeomID geomid = Survey::GeometryManager::cUndefGeomID();
		TypeSet<DataPointSet::DataRow> startset;
		BufferStringSet valnms;
		valnms.add("z2");
		DataPointSet* dtps = new DataPointSet( startset, valnms, true );

		linepar->get( sKey::GeomID(), geomid );
		hsamp.start_.inl() = hsamp.stop_.inl() = 0;
		StepInterval<int> trcrg;
		linepar->get( sKey::TrcRange(), trcrg );
		hsamp.setCrlRange( trcrg );
		HorizonUtils::getWantedPos2D( strm, midset, dtps,
					      hsamp, extraz, geomid );
		SeisTrcBuf seisoutp( false );
		uiString uierrmsg;
		mSetEngineMan()
		aem.setGeomID( geomid );
		Processor* proc = aem.create2DVarZOutput( uierrmsg, pars(),
				dtps, outval, zboundsset ? &zbounds : nullptr );
		if ( !proc )
		    mErrRet( uierrmsg.getFullString() );

		if ( !process(strm,proc,is2d,attribrefs,outpid,&seisoutp) )
		    return false;

		delete dtps;
	    }
	}
	else
	{
	    BinIDValueSet bivs(2,false);
	    PtrMan<Pos::Provider> provider = Pos::Provider::make( *mmprocrange,
								  false );
	    HorizonUtils::getWantedPositions( strm, midset, bivs,
				hsamp, extraz, nrinterpsamp, mainhoridx,
				extrawidth, provider );
	    SeisTrcBuf seisoutp( false );
	    uiString uierrmsg;
	    mSetEngineMan()
	    Processor* proc = aem.createTrcSelOutput( uierrmsg, bivs, seisoutp,
				outval, zboundsset ? &zbounds : nullptr );
	    if ( !proc )
		mErrRet( uierrmsg.getFullString() );

	    if ( !process(strm,proc,is2d,attribrefs,outpid,&seisoutp) )
		return false;
	}
    }

    strm << "Successfully saved data." << od_newline;

    deepErase(midset);
    deepUnRef( objects );

    return true;
}
