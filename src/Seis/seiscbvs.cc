/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 24-1-2001
 * FUNCTION : CBVS Seismic data translator
-*/

static const char* rcsID = "$Id: seiscbvs.cc,v 1.40 2003-10-15 15:15:54 bert Exp $";

#include "seiscbvs.h"
#include "seisinfo.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "cbvsreadmgr.h"
#include "cbvswritemgr.h"
#include "iostrm.h"
#include "cubesampling.h"
#include "iopar.h"
#include "binidselimpl.h"
#include "uidset.h"
#include "survinfoimpl.h"
#include "strmprov.h"
#include "separstr.h"
#include "filegen.h"


const IOPar& CBVSSeisTrcTranslator::datatypeparspec = new IOPar( "CBVS option");


CBVSSeisTrcTranslator::CBVSSeisTrcTranslator( const char* nm, const char* unm )
	: SeisTrcTranslator(nm,unm)
	, headerdone(false)
	, donext(false)
	, forread(true)
	, storinterps(0)
	, userawdata(0)
	, samedatachar(0)
	, blockbufs(0)
	, stptrs(0)
	, tdptrs(0)
	, preseldatatype(0)
	, rdmgr(0)
	, wrmgr(0)
	, nrdone(0)
    	, minimalhdrs(false)
    	, brickspec(*new VBrickSpec)
{
}


CBVSSeisTrcTranslator::~CBVSSeisTrcTranslator()
{
    cleanUp();
    delete &brickspec;
}


void CBVSSeisTrcTranslator::cleanUp()
{
    SeisTrcTranslator::cleanUp();
    headerdone = false;
    donext =false;
    nrdone = 0;
    destroyVars();
}


void CBVSSeisTrcTranslator::destroyVars()
{
    delete rdmgr; rdmgr = 0;
    delete wrmgr; wrmgr = 0;

    if ( !blockbufs ) return;

    const int nrcomps = nrSelComps();
    for ( int idx=0; idx<nrcomps; idx++ )
    {
	delete [] blockbufs[idx];
	delete storinterps[idx];
    }

    delete [] blockbufs; blockbufs = 0;
    delete [] storinterps; storinterps = 0;
    delete [] comps; comps = 0;
    delete [] userawdata; userawdata = 0;
    delete [] samedatachar; samedatachar = 0;
    delete [] stptrs; stptrs = 0;
    delete [] tdptrs; tdptrs = 0;
}


bool CBVSSeisTrcTranslator::getFileName( BufferString& fnm )
{
    if ( !conn || !conn->ioobj )
    {
	if ( !conn )
	    { errmsg = "Cannot reconstruct file name"; return false; }

	mDynamicCastGet(StreamConn*,strmconn,conn)
	if ( !strmconn )
	    { errmsg = "Wrong connection from Object Manager"; return false; }
	fnm = strmconn->fileName();
	return true;
    }

    mDynamicCastGet(IOStream*,iostrm,conn->ioobj)
    if ( !iostrm )
	{ errmsg = "Object manager provides wrong type"; return false; }

    // Catch the 'stdin' pretty name (currently "Std-IO")
    StreamProvider sp;
    fnm = iostrm->getExpandedName(true);
    if ( fnm == sp.fullName() )
	fnm = StreamProvider::sStdIO;

    conn->close();
    return true;
}


bool CBVSSeisTrcTranslator::initRead_()
{
    forread = true;
    BufferString fnm; if ( !getFileName(fnm) ) return false;

    rdmgr = new CBVSReadMgr( fnm );
    if ( rdmgr->failed() )
	{ errmsg = rdmgr->errMsg(); return false; }

    minimalhdrs = !rdmgr->hasAuxInfo();
    const int nrcomp = rdmgr->nrComponents();
    const CBVSInfo& info = rdmgr->info();
    for ( int idx=0; idx<nrcomp; idx++ )
    {
	BasicComponentInfo& cinf = *info.compinfo[idx];
	addComp( cinf.datachar, cinf.sd, cinf.nrsamples, cinf.name(),
		 cinf.scaler, cinf.datatype );
    }
    pinfo.usrinfo = info.usertext;
    pinfo.stdinfo = info.stdtext;
    pinfo.nr = info.seqnr;
    pinfo.binidsampling.start = info.geom.start;
    pinfo.binidsampling.stop = info.geom.stop;
    pinfo.binidsampling.step = info.geom.step;
    return true;
}


bool CBVSSeisTrcTranslator::initWrite_( const SeisTrc& trc )
{
    forread = false;

    for ( int idx=0; idx<trc.data().nrComponents(); idx++ )
    {
	DataCharacteristics dc(trc.data().getInterpreter(idx)->dataChar());
	BufferString nm( "Component " );
	nm += idx+1;
	mDynamicCastGet(const LinScaler*,sc,trc.scaler(idx))
	addComp( dc, trc.samplingData(idx), trc.size(idx), nm, sc );
	if ( preseldatatype )
	    tarcds[idx]->datachar = DataCharacteristics(
			(DataCharacteristics::UserType)preseldatatype );
    }

    return true;
}


void CBVSSeisTrcTranslator::calcSamps()
{
    const int nrcomps = tarcds.size();
    const int nrselcomps = nrSelComps();
    userawdata = new bool [nrselcomps];

    useinpsd = true;
    int iselc = -1;
    for ( int ic=0; ic<nrcomps; ic++ )
    {
	if ( tarcds[ic]->destidx < 0 ) continue;
	iselc++;

	userawdata[iselc] = true;
	// snap outcds[iselc]->sd.step
	if ( !iselc )
	{
	    float stepratio = outcds[iselc]->sd.step / inpcds[iselc]->sd.step;
	    samps.step = mNINT(stepratio);
	    if ( samps.step < 1 ) samps.step = 1;
	}
	if ( samps.step != 1 )
	    { useinpsd = userawdata[iselc] = false; }

	TargetComponentData& outcd = *outcds[iselc];
	const ComponentData& inpcd = *inpcds[iselc];
	if ( iselc )
	{
	    outcd.sd.start = outcds[0]->sd.start;
	    outcd.sd.step = outcds[0]->sd.step;
	    outcd.nrsamples = outcds[0]->nrsamples;
	    continue;
	}

	// snap outcd.sd.start
	float diff = outcd.sd.start - inpcd.sd.start;
	diff /= inpcd.sd.step;
	int idiff = mNINT(diff);
	if ( idiff ) useinpsd = false;
	const float outstart = inpcd.sd.start + idiff * inpcd.sd.step;

	float outstop = outcd.sd.start + outcd.sd.step * outcd.nrsamples;
	const float instop = inpcd.sd.start + inpcd.sd.step * inpcd.nrsamples;
	if ( outstop > instop ) outstop = instop;
	const float outstep = samps.step * inpcd.sd.step;
	float fnrsamps = (outstop - outstart) / outstep;

	outcd.sd.start = outstart;
	outcd.sd.step = outstep;
	outcd.nrsamples = mNINT(fnrsamps);

	float fsampnr = (outstart - inpcd.sd.start) / inpcd.sd.step;
	samps.start = mNINT(fsampnr);
	samps.stop = samps.start + (outcd.nrsamples-1) * samps.step;
	if ( samps.start < 0 ) samps.start = 0;
	if ( samps.stop < 0 ) samps.stop = 0;
	if ( samps.start >= inpcd.nrsamples )
	    samps.start = inpcd.nrsamples - 1;
	if ( samps.stop >= inpcd.nrsamples )
	    samps.stop = inpcd.nrsamples - 1;
	assign( cbvssamps, samps );

	outcd.nrsamples = (samps.stop - samps.start) / samps.step + 1;
	outcd.sd.start = inpcd.sd.start + samps.start * inpcd.sd.step;
    }
}


bool CBVSSeisTrcTranslator::commitSelections_()
{
    const int nrcomps = nrSelComps();
    if ( forread )
    {
	CubeSampling cs;
	if ( trcsel && trcsel->bidsel && trcsel->bidsel->type() < 2 )
	{
	    const BinIDRange& br = *(BinIDRange*)trcsel->bidsel;
	    BinID start = br.start; BinID stop = br.stop;
	    start.inl -= br.stepOut().inl; stop.inl += br.stepOut().inl;
	    start.crl -= br.stepOut().crl; stop.crl += br.stepOut().crl;
	    cs.hrg.start = start; cs.hrg.stop = stop;
	}
	cs.zrg.start = outcds[0]->sd.start;
	cs.zrg.stop = cs.zrg.start
	    	    + outcds[0]->sd.step * (outcds[0]->nrsamples - 1);

	if ( !rdmgr->pruneReaders( cs ) )
	    { errmsg = "Input contains no relevant data"; return false; }

	const BasicComponentInfo& ci = *rdmgr->info().compinfo[0];
	for ( int idx=0; idx<nrcomps; idx++ )
	{
	    inpcds[idx]->sd.start = ci.sd.start;
	    inpcds[idx]->nrsamples = ci.nrsamples;
	}
    }

    calcSamps();
    samedatachar = new bool [nrcomps];
    storinterps = new TraceDataInterpreter* [nrcomps];
    stptrs = new unsigned char* [nrcomps];
    tdptrs = new unsigned char* [nrcomps];
    for ( int idx=0; idx<nrcomps; idx++ )
    {
	samedatachar[idx] = inpcds[idx]->datachar == outcds[idx]->datachar;
	userawdata[idx] = samedatachar[idx] && userawdata[idx];
	if ( idx )
	    outcds[idx]->nrsamples = actualsz;
	else
	    actualsz = outcds[idx]->nrsamples;
	storinterps[idx] = new TraceDataInterpreter(
                  forread ? inpcds[idx]->datachar : outcds[idx]->datachar );
    }

    blockbufs = new unsigned char* [nrcomps];
    for ( int iselc=0; iselc<nrcomps; iselc++ )
    {
	int bufsz = inpcds[iselc]->nrsamples;
	if ( outcds[iselc]->nrsamples > bufsz )
	    bufsz = outcds[iselc]->nrsamples;
	bufsz += 1;
	int nbts = inpcds[iselc]->datachar.nrBytes();
	if ( outcds[iselc]->datachar.nrBytes() > nbts )
	    nbts = outcds[iselc]->datachar.nrBytes();

	blockbufs[iselc] = new unsigned char [ nbts * bufsz ];
	if ( !blockbufs[iselc] ) { errmsg = "Out of memory"; return false; }
    }

    comps = new bool [tarcds.size()];
    for ( int idx=0; idx<tarcds.size(); idx++ )
	comps[idx] = tarcds[idx]->destidx >= 0;

    if ( !forread )
	return startWrite();

    return !trcsel || !trcsel->bidsel
	|| trcsel->bidsel->includes(rdmgr->binID())
	? true : toNext();
}


bool CBVSSeisTrcTranslator::toNext()
{
    if ( !trcsel || !trcsel->bidsel )
	return rdmgr->toNext();

    const CBVSInfo& info = rdmgr->info();
    if ( info.nrtrcsperposn > 1 )
    {
	bool rv = rdmgr->toNext();
	if ( !rv ) return false;
	if ( trcsel->bidsel->includes(rdmgr->binID()) )
	    return true;
    }

    BinID nextbid = rdmgr->nextBinID();
    if ( nextbid == BinID(0,0) )
	return false;
    if ( trcsel->bidsel->includes(nextbid) )
	return rdmgr->toNext();

    // find next requested BinID
    while ( 1 )
    {

	while ( 1 )
	{
	    int res = trcsel->bidsel->excludes( nextbid );
	    if ( !res ) break;

	    if ( res%256 == 2 )
		{ if ( !info.geom.toNextInline(nextbid) ) return false; }
	    else if ( !info.geom.toNextBinID(nextbid) )
		return false;
	}

	if ( goTo(nextbid) ) break;
	if ( !info.geom.toNextBinID(nextbid) )
	    return false;

    }

    return true;
}


bool CBVSSeisTrcTranslator::goTo( const BinID& bid )
{
    if ( rdmgr->goTo(bid) )
    {
	headerdone = false;
	donext = false;
	return true;
    }
    return false;
}


bool CBVSSeisTrcTranslator::readInfo( SeisTrcInfo& ti )
{
    if ( !storinterps && !commitSelections() ) return false;
    if ( headerdone ) return true;

    donext = donext ||   ( trcsel && trcsel->bidsel
			&& !trcsel->bidsel->includes(rdmgr->binID()) );

    if ( donext && !toNext() ) return false;
    donext = true;

    if ( !rdmgr->getAuxInfo(auxinf) )
	return false;

    ti.getFrom( auxinf );
    if ( !useinpsd )
	ti.sampling.start = outcds[0]->sd.start;
    ti.sampling.step = outcds[0]->sd.step;
    ti.nr = ++nrdone;

    if ( !rdmgr->info().auxinfosel.coord )
	ti.coord = SI().transform( ti.binid );
    else if ( ti.binid.inl == 0 && ti.binid.crl == 0 )
	ti.binid = SI().transform( ti.coord );

    return (headerdone = true);
}


bool CBVSSeisTrcTranslator::read( SeisTrc& trc )
{
    if ( !headerdone && !readInfo(trc.info()) )
	return false;

    prepareComponents( trc, actualsz );
    const CBVSInfo& info = rdmgr->info();
    const int nselc = nrSelComps();
    for ( int iselc=0; iselc<nselc; iselc++ )
    {
	const BasicComponentInfo& ci = *info.compinfo[ selComp(iselc) ];
	trc.setScaler( ci.scaler, iselc );
	tdptrs[iselc] = trc.data().getComponent( iselc )->data();
	stptrs[iselc] = userawdata[iselc] ? tdptrs[iselc] : blockbufs[iselc];
    }

    if ( !rdmgr->fetch( (void**)stptrs, comps, &cbvssamps ) )
    {
	errmsg = rdmgr->errMsg();
	return false;
    }

    for ( int iselc=0; iselc<nselc; iselc++ )
    {
	if ( userawdata[iselc] ) continue;

	if ( samedatachar[iselc] )
	{
	    // Take each n-th sample
	    for ( int outsmp=0; outsmp<outcds[iselc]->nrsamples; outsmp++ )
	    {
		memcpy( tdptrs[iselc], stptrs[iselc],
			(int)inpcds[iselc]->datachar.nrBytes() );
		stptrs[iselc] += (int)inpcds[iselc]->datachar.nrBytes()
				* samps.step;
		tdptrs[iselc] += (int)inpcds[iselc]->datachar.nrBytes();
	    }
	}
	else
	{
	    // Convert data into other format
	    for ( int outsmp=0,inp_samp=0; outsmp<outcds[iselc]->nrsamples;
		  outsmp++ )
	    {
		trc.set( outsmp,
			 storinterps[iselc]->get( stptrs[iselc], inp_samp ),
			 iselc);
		inp_samp += samps.step;
	    }
	}

    }

    headerdone = false;
    return true;
}


bool CBVSSeisTrcTranslator::skip( int )
{
    if ( !rdmgr->skip(true) ) return false;
    donext = false;
    headerdone = false;
    return true;
}


BinID2Coord CBVSSeisTrcTranslator::getTransform() const
{
    if ( !rdmgr || !rdmgr->nrReaders() )
	return SI().is3D() ? SI3D().binID2Coord() : BinID2Coord();
    return rdmgr->info().geom.b2c;
}


bool CBVSSeisTrcTranslator::startWrite()
{
    BufferString fnm; if ( !getFileName(fnm) ) return false;

    CBVSInfo info;
    info.auxinfosel.setAll( !minimalhdrs );
    info.geom.fullyrectandreg = false;
    if ( SI().is3D() )
	info.geom.b2c = SI3D().binID2Coord();
    info.stdtext = pinfo.stdinfo;
    info.usertext = pinfo.usrinfo;
    for ( int idx=0; idx<nrSelComps(); idx++ )
	info.compinfo += new BasicComponentInfo(*outcds[idx]);

    wrmgr = new CBVSWriteMgr( fnm, info, &auxinf, &brickspec );
    if ( wrmgr->failed() )
	{ errmsg = wrmgr->errMsg(); return false; }

    return true;
}


bool CBVSSeisTrcTranslator::writeTrc_( const SeisTrc& trc )
{
    for ( int iselc=0; iselc<nrSelComps(); iselc++ )
    {
	tdptrs[iselc] = const_cast<unsigned char*>(
			trc.data().getComponent(selComp(iselc))->data() );
	stptrs[iselc] = userawdata[iselc] ? tdptrs[iselc] : blockbufs[iselc];
	const Scaler* sc = trc.scaler( selComp(iselc) );
	if ( !userawdata[iselc] || sc )
	{
	    if ( samedatachar[iselc] )
	    {
		for ( int outsmp=0; outsmp<outcds[iselc]->nrsamples; outsmp++ )
		memcpy( stptrs[iselc], tdptrs[iselc],
			(int)inpcds[iselc]->datachar.nrBytes() );
		tdptrs[iselc] += (int)inpcds[iselc]->datachar.nrBytes()
				* samps.step;
		stptrs[iselc] += (int)inpcds[iselc]->datachar.nrBytes();
	    }
	    else if ( !sc )
	    {
		const TraceDataInterpreter* inpinterp
			= trc.data().getInterpreter(selComp(iselc));
		for ( int outsmp=0,inp_samp=0; outsmp<outcds[iselc]->nrsamples;
		  outsmp++ )
		{
		    storinterps[iselc]->put( stptrs[iselc], outsmp,
			inpinterp->get( tdptrs[iselc], inp_samp ) );
		    inp_samp += samps.step;
		}
	    }
	    else
	    {
		float t = outcds[iselc]->sd.start;
		int icomp = selComp(iselc);
		for ( int outsmp=0; outsmp<outcds[iselc]->nrsamples; outsmp++ )
		{
		    storinterps[iselc]->put( stptrs[iselc], outsmp,
			    		     trc.getValue(t,icomp) );
		    t += outcds[iselc]->sd.step;
		}
	    }
	}
    }

    trc.info().putTo( auxinf );
    if ( !wrmgr->put( (void**)stptrs ) )
	{ errmsg = wrmgr->errMsg(); return false; }

    return true;
}


const IOPar* CBVSSeisTrcTranslator::parSpec( Conn::State ) const
{
    if ( !datatypeparspec.size() )
    {
	FileNameString fms;
	const char* ptr = DataCharacteristics::UserTypeNames[0];
	for ( int idx=0; DataCharacteristics::UserTypeNames[idx]
		     && *DataCharacteristics::UserTypeNames[idx]; idx++ )
	    fms += DataCharacteristics::UserTypeNames[idx];
	IOPar& ps = const_cast<IOPar&>( datatypeparspec );
	ps.set( "Data storage", fms );
    }
    return &datatypeparspec;
}


void CBVSSeisTrcTranslator::usePar( const IOPar& iopar )
{
    SeisTrcTranslator::usePar( iopar );

    const char* res = iopar.find( datatypeparspec.getKey(0) );
    if ( res && *res )
	preseldatatype = (DataCharacteristics::UserType)(*res-'0');

    res = iopar.find( "Optimized direction" );
    if ( res && *res )
    {
	brickspec.setStd( *res == 'H' );
	if ( *res == 'H' && *res && *(res+1) == '`' )
	{
	    FileMultiString fms( res + 2 );
	    const int sz = fms.size();
	    int tmp = atoi( fms[0] );
	    if ( tmp > 0 )
		brickspec.nrsamplesperslab = tmp < 100000 ? tmp : 100000;
	    if ( sz > 1 )
	    {
		tmp = atoi( fms[1] );
		if ( tmp > 0 )
		    brickspec.maxnrslabs = tmp;
	    }
	}
    }
}


#define mImplStart(fn) \
    if ( !ioobj || strcmp(ioobj->translator(),"CBVS") ) return false; \
    mDynamicCastGet(const IOStream*,iostrm,ioobj) \
    if ( !iostrm ) return false; \
    if ( iostrm->isMulti() ) \
	return const_cast<IOStream*>(iostrm)->fn; \
 \
    BufferString pathnm = iostrm->dirName(); \
    BufferString basenm = iostrm->fileName()

#define mImplLoopStart \
	StreamProvider sp( CBVSIOMgr::getFileName(basenm,nr) ); \
	sp.addPathIfNecessary( pathnm ); \
	if ( !sp.exists(true) ) \
	    return true;


bool CBVSSeisTrcTranslator::implRemove( const IOObj* ioobj ) const
{
    mImplStart(implRemove());

    for ( int nr=0; ; nr++ )
    {
	mImplLoopStart;

	if ( !sp.remove(false) )
	    return nr ? true : false;
    }
}


bool CBVSSeisTrcTranslator::implRename( const IOObj* ioobj, const char* newnm,
       					const CallBack* cb ) const
{
    mImplStart( implRename(newnm) );

    bool rv = true;
    for ( int nr=0; ; nr++ )
    {
	mImplLoopStart;

	StreamProvider spnew( CBVSIOMgr::getFileName(newnm,nr) );
	spnew.addPathIfNecessary( pathnm );
	if ( !sp.rename(spnew.fileName(),cb) )
	    rv = false;
    }
}


bool CBVSSeisTrcTranslator::implSetReadOnly( const IOObj* ioobj, bool yn ) const
{
    mImplStart( implSetReadOnly(yn) );

    bool rv = true;
    for ( int nr=0; ; nr++ )
    {
	mImplLoopStart;

	if ( !sp.setReadOnly(yn) )
	    rv = false;
    }
}
