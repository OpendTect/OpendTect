/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 24-1-2001
 * FUNCTION : Segy-like trace translator
-*/

static const char* rcsID = "$Id: seiscbvs.cc,v 1.15 2001-12-11 14:24:02 bert Exp $";

#include "seiscbvs.h"
#include "seisinfo.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "cbvsreadmgr.h"
#include "cbvswritemgr.h"
#include "iostrm.h"
#include "iopar.h"
#include "binidselimpl.h"
#include "uidset.h"
#include "survinfo.h"
#include "strmprov.h"
#include "filegen.h"


UserIDSet CBVSSeisTrcTranslator::datatypeparspec(
		DataCharacteristics::UserTypeNames,"Data storage");


CBVSSeisTrcTranslator::CBVSSeisTrcTranslator( const char* nm )
	: SeisTrcTranslator(nm)
	, headerdone(false)
	, donext(false)
	, forread(true)
	, storinterps(0)
	, samps(0)
	, cbvssamps(0)
	, comps(0)
	, userawdata(0)
	, samedatachar(0)
	, actualsz(0)
	, blockbufs(0)
	, stptrs(0)
	, tdptrs(0)
	, preseldatatype(0)
	, rdmgr(0)
	, wrmgr(0)
	, nrdone(0)
{
}


CBVSSeisTrcTranslator::~CBVSSeisTrcTranslator()
{
    cleanUp();
}


void CBVSSeisTrcTranslator::close()
{
    cleanUp();
    SeisTrcTranslator::close();
}


void CBVSSeisTrcTranslator::cleanUp()
{
    headerdone = false;
    donext =false;
    nrdone = 0;
    destroyVars();
    SeisTrcTranslator::cleanUp();
}


void CBVSSeisTrcTranslator::destroyVars()
{
    if ( !blockbufs ) return;

    const int nrcomps = nrSelComps();
    for ( int idx=0; idx<nrcomps; idx++ )
    {
	delete [] blockbufs[idx];
	delete storinterps[idx];
    }

    delete [] blockbufs; blockbufs = 0;
    delete [] storinterps; storinterps = 0;
    delete [] samps; samps = 0;
    delete [] cbvssamps; cbvssamps = 0;
    delete [] comps; comps = 0;
    delete [] userawdata; userawdata = 0;
    delete [] samedatachar; samedatachar = 0;
    delete [] actualsz; actualsz = 0;
    delete [] stptrs; stptrs = 0;
    delete [] tdptrs; tdptrs = 0;

    delete rdmgr;
    delete wrmgr;
}


bool CBVSSeisTrcTranslator::getFileName( BufferString& fnm )
{
    if ( !conn || !conn->ioobj )
	{ errmsg = "Cannot reconstruct file name"; return false; }

    // Catch the 'stdin' pretty name (currently "Std-IO")
    StreamProvider sp;
    fnm = conn->ioobj->fullUserExpr(true);
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
    pinfo.range.start = info.geom.start;
    pinfo.range.stop = info.geom.stop;
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
    samps = new StepInterval<int> [nrselcomps];
    cbvssamps = new Interval<int> [nrcomps];
    userawdata = new bool [nrselcomps];

    useinpsd = true;
    int iselc = -1;
    for ( int ic=0; ic<nrcomps; ic++ )
    {
	if ( tarcds[ic]->destidx < 0 ) continue;
	iselc++;

	userawdata[iselc] = true;
	// snap outcds[iselc]->sd.step
	float stepratio = outcds[iselc]->sd.step / inpcds[iselc]->sd.step;
	samps[iselc].step = mNINT(stepratio);
	if ( samps[iselc].step < 1 ) samps[iselc].step = 1;
	float outstep = samps[iselc].step * inpcds[iselc]->sd.step;
	if ( samps[iselc].step != 1 ) { useinpsd = userawdata[iselc] = false; }

	// snap outcds[iselc]->sd.start
	float diff = outcds[iselc]->sd.start - inpcds[iselc]->sd.start;
	diff /= inpcds[iselc]->sd.step;
	int idiff = mNINT(diff);
	if ( idiff ) useinpsd = false;
	float outstart = inpcds[iselc]->sd.start
		       + idiff * inpcds[iselc]->sd.step;

	float outstop = outcds[iselc]->sd.start
			 + outcds[iselc]->sd.step * outcds[iselc]->nrsamples;
	const float instop = inpcds[iselc]->sd.start
			   + inpcds[iselc]->sd.step * inpcds[iselc]->nrsamples;
	if ( outstop > instop ) outstop = instop;
	float fnrsamps = (outstop - outstart) / outstep;

	outcds[iselc]->sd.start = outstart;
	outcds[iselc]->sd.step = outstep;
	outcds[iselc]->nrsamples = mNINT(fnrsamps);

	float fsampnr = (outstart - inpcds[iselc]->sd.start)
	    		/ inpcds[iselc]->sd.step;
	samps[iselc].start = mNINT(fsampnr);
	samps[iselc].stop = samps[iselc].start
	    		  + (outcds[iselc]->nrsamples-1) * samps[iselc].step;
	if ( samps[iselc].start < 0 ) samps[iselc].start = 0;
	if ( samps[iselc].stop < 0 ) samps[iselc].stop = 0;
	if ( samps[iselc].start >= inpcds[iselc]->nrsamples )
	    samps[iselc].start = inpcds[iselc]->nrsamples - 1;
	if ( samps[iselc].stop >= inpcds[iselc]->nrsamples )
	    samps[iselc].stop = inpcds[iselc]->nrsamples - 1;
	assign( cbvssamps[ic], samps[iselc] );

	outcds[iselc]->nrsamples = (samps[iselc].stop - samps[iselc].start)
	    			 / samps[iselc].step + 1;
	outcds[iselc]->sd.start = inpcds[iselc]->sd.start
	    			+ samps[iselc].start * inpcds[iselc]->sd.step;
    }
}


bool CBVSSeisTrcTranslator::commitSelections_()
{
    const int nrcomps = nrSelComps();

    calcSamps();
    samedatachar = new bool [nrcomps];
    actualsz = new int [nrcomps];
    storinterps = new TraceDataInterpreter* [nrcomps];
    stptrs = new unsigned char* [nrcomps];
    tdptrs = new unsigned char* [nrcomps];
    for ( int idx=0; idx<nrcomps; idx++ )
    {
	samedatachar[idx] = inpcds[idx]->datachar == outcds[idx]->datachar;
	userawdata[idx] = samedatachar[idx] && userawdata[idx];
	actualsz[idx] = outcds[idx]->nrsamples;
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

    BinID nextbid = rdmgr->nextBinID();
    if ( nextbid == BinID(0,0) )
	return false;
    if ( trcsel->bidsel->includes(nextbid) )
	return rdmgr->toNext();

    // find next requested BinID
    const CBVSInfo& info = rdmgr->info();
    while ( 1 )
    {
	int res = trcsel->bidsel->excludes( nextbid );
	if ( !res ) break;

	if ( res%256 == 2 )
	    { if ( !info.geom.toNextInline(nextbid) ) return false; }
	else if ( !info.geom.toNextBinID(nextbid) )
	    return false;
    }

    return goTo( nextbid );
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
    if ( !storinterps ) commitSelections();
    if ( headerdone ) return true;

    donext = donext ||   ( trcsel && trcsel->bidsel
			&& !trcsel->bidsel->includes(rdmgr->binID()) );

    if ( donext && !toNext() ) return false;
    donext = true;

    static CBVSInfo::ExplicitData expldat;
    if ( !rdmgr->getHInfo(expldat) )
    {
	errmsg = "Cannot get header info";
	return false;
    }
    ti.nr = ++nrdone;
    ti.binid = expldat.binid;
    ti.sampling.start = useinpsd ? expldat.startpos : outcds[0]->sd.start;
    ti.sampling.step = outcds[0]->sd.step;
    ti.coord = expldat.coord;
    ti.offset = expldat.offset;
    ti.azimuth = expldat.azimuth;
    ti.pick = expldat.pick;
    ti.refpos = expldat.refpos;

    if ( !rdmgr->info().explinfo.coord )
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
    for ( int iselc=0; iselc<nrSelComps(); iselc++ )
    {
	const BasicComponentInfo& ci = *info.compinfo[ selComp(iselc) ];
	trc.setScaler( ci.scaler, iselc );
	tdptrs[iselc] = trc.data().getComponent( iselc )->data();
	stptrs[iselc] = userawdata[iselc] ? tdptrs[iselc] : blockbufs[iselc];
    }

    if ( !rdmgr->fetch( (void**)stptrs, comps, cbvssamps ) )
    {
	errmsg = rdmgr->errMsg();
	return false;
    }

    for ( int iselc=0; iselc<nrSelComps(); iselc++ )
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
				* samps[iselc].step;
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
		inp_samp += samps[iselc].step;
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


bool CBVSSeisTrcTranslator::startWrite()
{
    BufferString fnm; if ( !getFileName(fnm) ) return false;

    CBVSInfo info;
    info.explinfo.startpos = info.explinfo.coord = 
    info.explinfo.offset = info.explinfo.azimuth =
    info.explinfo.pick = info.explinfo.refpos = true;
    info.geom.fullyrectandreg = false;
    info.geom.b2c = SI().binID2Coord();
    info.stdtext = pinfo.stdinfo;
    info.usertext = pinfo.usrinfo;
    for ( int idx=0; idx<nrSelComps(); idx++ )
	info.compinfo += new BasicComponentInfo(*outcds[idx]);

    wrmgr = new CBVSWriteMgr( fnm, info, &expldat );
    if ( wrmgr->failed() )
	{ errmsg = wrmgr->errMsg(); return false; }

    return true;
}


bool CBVSSeisTrcTranslator::write( const SeisTrc& trc )
{
    if ( !storinterps ) commitSelections( &trc );

    expldat.binid = trc.info().binid;
    expldat.startpos = trc.info().sampling.start;
    expldat.coord = trc.info().coord;
    expldat.offset = trc.info().offset;
    expldat.azimuth = trc.info().azimuth;
    expldat.pick = trc.info().pick;
    expldat.refpos = trc.info().refpos;

    for ( int iselc=0; iselc<nrSelComps(); iselc++ )
    {
	tdptrs[iselc] = const_cast<unsigned char*>(
			trc.data().getComponent(selComp(iselc))->data() );
	stptrs[iselc] = userawdata[iselc] ? tdptrs[iselc] : blockbufs[iselc];
	if ( !userawdata[iselc] )
	{
	    if ( samedatachar[iselc] )
	    {
		for ( int outsmp=0; outsmp<outcds[iselc]->nrsamples; outsmp++ )
		memcpy( stptrs[iselc], tdptrs[iselc],
			(int)inpcds[iselc]->datachar.nrBytes() );
		tdptrs[iselc] += (int)inpcds[iselc]->datachar.nrBytes()
				* samps[iselc].step;
		stptrs[iselc] += (int)inpcds[iselc]->datachar.nrBytes();
	    }
	    else
	    {
		const TraceDataInterpreter* inpinterp
			= trc.data().getInterpreter(selComp(iselc));
		for ( int outsmp=0,inp_samp=0; outsmp<outcds[iselc]->nrsamples;
		  outsmp++ )
		{
		    storinterps[iselc]->put( stptrs[iselc], outsmp,
			inpinterp->get( tdptrs[iselc], inp_samp ) );
		    inp_samp += samps[iselc].step;
		}
	    }
	}
    }

    if ( !wrmgr->put( (void**)stptrs ) )
    {
	errmsg = wrmgr->errMsg();
	return false;
    }

    return true;
}


void CBVSSeisTrcTranslator::usePar( const IOPar* iopar )
{
    SeisTrcTranslator::usePar( iopar );
    if ( !iopar ) return;

    const char* res = (*iopar)[ (const char*)datatypeparspec.name() ];
    if ( *res )
	preseldatatype = (DataCharacteristics::UserType)(*res-'0');
}


int CBVSSeisTrcTranslator::implRemove( const IOObj* ioobj ) const
{
    if ( !ioobj || strcmp(ioobj->translator(),"CBVS") ) return NO;
    mDynamicCastGet(const IOStream*,iostrm,ioobj)
    if ( !iostrm ) return NO;

    BufferString pathnm = iostrm->dirName();
    BufferString basenm = iostrm->fileName();

    for ( int nr=0; ; nr++ )
    {
	StreamProvider sp( CBVSIOMgr::getFileName( basenm, nr ) );
	sp.addPathIfNecessary( pathnm );
	if ( !sp.exists(YES) )
	    return YES;
	if ( !sp.remove() )
	    return nr ? YES : NO;
    }
}
