/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 24-1-2001
 * FUNCTION : Segy-like trace translator
-*/

static const char* rcsID = "$Id: seiscbvs.cc,v 1.4 2001-05-25 18:26:01 bert Exp $";

#include "seiscbvs.h"
#include "seisinfo.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "cbvsreadmgr.h"
#include "cbvswritemgr.h"
#include "ioobj.h"
#include "iopar.h"
#include "binidselimpl.h"
#include "uidset.h"
#include "survinfo.h"


UserIDSet CBVSSeisTrcTranslator::datatypeparspec(
		DataCharacteristics::UserTypeNames,"Data storage");


CBVSSeisTrcTranslator::CBVSSeisTrcTranslator( const char* nm )
	: SeisTrcTranslator(nm)
	, headerdone(false)
	, donext(false)
	, forread(true)
	, storinterps(0)
	, samps(0)
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

    fnm = conn->ioobj->fullUserExpr(true);
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
		 cinf.scaler );
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
	addComp( dc, trc.samplingData(idx), trc.size(idx), nm,
		 dynamic_cast<const LinScaler*>(trc.scaler(idx)) );
	if ( preseldatatype )
	    tarcds[idx]->datachar = DataCharacteristics(
			(DataCharacteristics::UserType)preseldatatype );
    }

    return true;
}


void CBVSSeisTrcTranslator::calcSamps()
{
    const int nrcomps = nrSelComps();
    samps = new StepInterval<int> [nrcomps];
    userawdata = new bool [nrcomps];

    useinpsd = true;
    for ( int idx=0; idx<nrcomps; idx++ )
    {
	userawdata[idx] = true;
	// snap outcds[idx]->sd.step
	float stepratio = outcds[idx]->sd.step / inpcds[idx]->sd.step;
	samps[idx].step = mNINT(stepratio);
	if ( samps[idx].step < 1 ) samps[idx].step = 1;
	float outstep = samps[idx].step * inpcds[idx]->sd.step;
	if ( samps[idx].step != 1 ) { useinpsd = userawdata[idx] = false; }

	// snap outcds[idx]->sd.start
	float diff = outcds[idx]->sd.start - inpcds[idx]->sd.start;
	diff /= outcds[idx]->sd.step;
	int idiff = mNINT(diff);
	if ( idiff ) useinpsd = false;
	float outstart = inpcds[idx]->sd.start + idiff * inpcds[idx]->sd.step;

	float orgoutstop = outcds[idx]->sd.start
			 + outcds[idx]->sd.step * outcds[idx]->nrsamples;
	float fnrsamps = (orgoutstop - outstart) / outstep;

	outcds[idx]->sd.start = outstart;
	outcds[idx]->sd.step = outstep;
	outcds[idx]->nrsamples = mNINT(fnrsamps);

	float fstart = (outstart - inpcds[idx]->sd.start) / outstep;
	samps[idx].start = mNINT(fstart);
	samps[idx].stop = samps[idx].start + outcds[idx]->nrsamples - 1;
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
	actualsz[idx] = samps[idx].width() + 1;
	storinterps[idx] = new TraceDataInterpreter(
                  forread ? inpcds[idx]->datachar : outcds[idx]->datachar );
    }

    blockbufs = new unsigned char* [nrcomps];
    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	int bufsz = inpcds[icomp]->nrsamples;
	if ( outcds[icomp]->nrsamples > bufsz )
	    bufsz = outcds[icomp]->nrsamples;
	bufsz += 1;
	int nbts = inpcds[icomp]->datachar.nrBytes();
	if ( outcds[icomp]->datachar.nrBytes() > nbts )
	    nbts = outcds[icomp]->datachar.nrBytes();

	blockbufs[icomp] = new unsigned char [ nbts * bufsz ];
	if ( !blockbufs[icomp] ) { errmsg = "Out of memory"; return false; }
    }

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
    for ( int icomp=0; icomp<nrSelComps(); icomp++ )
    {
	const BasicComponentInfo& ci = *info.compinfo[ selComp(icomp) ];
	trc.setScaler( ci.scaler, icomp );
	tdptrs[icomp] = trc.data().getComponent(selComp(icomp))->data();
	stptrs[icomp] = userawdata[icomp] ? tdptrs[icomp] : blockbufs[icomp];
    }

    if ( !rdmgr->fetch( (void**)stptrs, samps ) )
    {
	errmsg = rdmgr->errMsg();
	return false;
    }

    for ( int icomp=0; icomp<nrSelComps(); icomp++ )
    {
	if ( userawdata[icomp] ) continue;

	if ( samedatachar[icomp] )
	{
	    // Take each n-th sample
	    for ( int outsmp=0; outsmp<outcds[icomp]->nrsamples; outsmp++ )
	    {
		memcpy( tdptrs[icomp], stptrs[icomp],
			(int)inpcds[icomp]->datachar.nrBytes() );
		stptrs[icomp] += (int)inpcds[icomp]->datachar.nrBytes()
				* samps[icomp].step;
		tdptrs[icomp] += (int)inpcds[icomp]->datachar.nrBytes();
	    }
	}
	else
	{
	    // Convert data into other format
	    for ( int outsmp=0,inp_samp=0; outsmp<outcds[icomp]->nrsamples;
		  outsmp++ )
	    {
		trc.set( outsmp,
			 storinterps[icomp]->get( stptrs[icomp], inp_samp ),
			 icomp);
		inp_samp += samps[icomp].step;
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
    info.explinfo.offset = info.explinfo.pick = 
    info.explinfo.refpos = true;
    info.geom.fullyrectandreg = false;
    info.stdtext = pinfo.stdinfo;
    info.usertext = pinfo.usrinfo;
    for ( int idx=0; idx<nrSelComps(); idx++ )
	info.compinfo += new BasicComponentInfo(*tarcds[idx]);

    wrmgr = new CBVSWriteMgr( fnm, info, &expldat );
    if ( wrmgr->failed() )
	{ errmsg = wrmgr->errMsg(); return false; }

    return true;
}


bool CBVSSeisTrcTranslator::write( const SeisTrc& trc )
{
    if ( !storinterps ) commitSelections();

    expldat.binid = trc.info().binid;
    expldat.startpos = trc.info().sampling.start;
    expldat.coord = trc.info().coord;
    expldat.offset = trc.info().offset;
    expldat.pick = trc.info().pick;
    expldat.refpos = trc.info().refpos;

    for ( int icomp=0; icomp<nrSelComps(); icomp++ )
    {
	tdptrs[icomp] = const_cast<unsigned char*>(
			trc.data().getComponent(selComp(icomp))->data() );
	stptrs[icomp] = userawdata[icomp] ? tdptrs[icomp] : blockbufs[icomp];
	if ( !userawdata[icomp] )
	{
	    if ( samedatachar[icomp] )
	    {
		for ( int outsmp=0; outsmp<outcds[icomp]->nrsamples; outsmp++ )
		memcpy( stptrs[icomp], tdptrs[icomp],
			(int)inpcds[icomp]->datachar.nrBytes() );
		tdptrs[icomp] += (int)inpcds[icomp]->datachar.nrBytes()
				* samps[icomp].step;
		stptrs[icomp] += (int)inpcds[icomp]->datachar.nrBytes();
	    }
	    else
	    {
		const TraceDataInterpreter* inpinterp
			= trc.data().getInterpreter(selComp(icomp));
		for ( int outsmp=0,inp_samp=0; outsmp<outcds[icomp]->nrsamples;
		  outsmp++ )
		{
		    storinterps[icomp]->put( stptrs[icomp], outsmp,
			inpinterp->get( tdptrs[icomp], inp_samp ) );
		    inp_samp += samps[icomp].step;
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
