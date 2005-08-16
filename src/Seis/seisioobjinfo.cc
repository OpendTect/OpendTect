/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : June 2005
-*/

static const char* rcsID = "$Id: seisioobjinfo.cc,v 1.4 2005-08-16 17:10:17 cvsbert Exp $";

#include "seisioobjinfo.h"
#include "seistrcsel.h"
#include "seistrctr.h"
#include "seispsioprov.h"
#include "seiscbvs.h"
#include "seis2dline.h"
#include "ptrman.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "conn.h"
#include "survinfo.h"
#include "binidselimpl.h"
#include "cubesampling.h"
#include "linekey.h"
#include "errh.h"


SeisIOObjInfo::SeisIOObjInfo( const IOObj* ioobj )
    	: ioobj_(ioobj ? ioobj->clone() : 0)		{ setType(); }
SeisIOObjInfo::SeisIOObjInfo( const IOObj& ioobj )
    	: ioobj_(ioobj.clone())				{ setType(); }
SeisIOObjInfo::SeisIOObjInfo( const MultiID& id )
    	: ioobj_(IOM().get(id))				{ setType(); }


SeisIOObjInfo::SeisIOObjInfo( const SeisIOObjInfo& sii )
    	: geomtype_(sii.geomtype_)
	, bad_(sii.bad_)
{
    ioobj_ = sii.ioobj_ ? sii.ioobj_->clone() : 0;
}


SeisIOObjInfo& SeisIOObjInfo::operator =( const SeisIOObjInfo& sii )
{
    if ( &sii != this )
    {
	delete ioobj_;
	ioobj_ = sii.ioobj_ ? sii.ioobj_->clone() : 0;
    	geomtype_ = sii.geomtype_;
    	bad_ = sii.bad_;
    }
    return *this;
}


void SeisIOObjInfo::setType()
{
    bad_ = !ioobj_;
    if ( bad_ ) return;

    const BufferString trgrpnm( ioobj_->group() );
    bool isps = false;
    if ( trgrpnm == mTranslGroupName(SeisPS) )
	isps = true;
    else if ( trgrpnm != mTranslGroupName(SeisTrc) )
	{ bad_ = true; return; }
    ioobj_->pars().getYN( SeisTrcTranslator::sKeyIsPS, isps );

    const bool is2d = SeisTrcTranslator::is2D( *ioobj_, false );
    geomtype_ = isps ? (is2d ? Seis::LinePS : Seis::VolPS)
		     : (is2d ? Seis::Line : Seis::Vol);
}


SeisIOObjInfo::SpaceInfo::SpaceInfo( int ns, int ntr, int bps )
	: expectednrsamps(ns)
	, expectednrtrcs(ntr)
	, maxbytespsamp(bps)
{
    if ( expectednrsamps < 0 )
	expectednrsamps = SI().zRange(false).nrSteps() + 1;
    if ( expectednrtrcs < 0 )
	expectednrtrcs = SI().sampling(false).hrg.totalNr();
}

#define mChk(ret) if ( bad_ ) return ret

int SeisIOObjInfo::expectedMBs( const SpaceInfo& si ) const
{
    mChk(-1);

    if ( isPS() )
    {
	pErrMsg("TODO: no space estimate for PS");
	return -1;
    }

    Translator* tr = ioobj_->getTranslator();
    mDynamicCastGet(SeisTrcTranslator*,sttr,tr)
    if ( !sttr )
	{ pErrMsg("No Translator!"); return -1; }

    if ( si.expectednrtrcs < 0 || mIsUndefInt(si.expectednrtrcs) )
	return -1;

    int overhead = sttr->bytesOverheadPerTrace();
    delete tr;
    double sz = si.expectednrsamps;
    sz *= si.maxbytespsamp;
    sz = (sz + overhead) * si.expectednrtrcs;

    static const double bytes2mb = 9.53674e-7;
    return (int)((sz * bytes2mb) + .5);
}


bool SeisIOObjInfo::getRanges( CubeSampling& cs ) const
{
    mChk(false);
    return SeisTrcTranslator::getRanges( *ioobj_, cs );
}


bool SeisIOObjInfo::getBPS( int& bps, int icomp ) const
{
    mChk(false);
    if ( isPS() )
    {
	pErrMsg("TODO: no BPS for PS");
	return -1;
    }

    Translator* tr = ioobj_->getTranslator();
    mDynamicCastGet(SeisTrcTranslator*,sttr,tr)
    if ( !sttr )
	{ pErrMsg("No Translator!"); return false; }

    Conn* conn = ioobj_->getConn( Conn::Read );
    bool isgood = sttr->initRead(conn);
    bps = 0;
    if ( isgood )
    {
	ObjectSet<SeisTrcTranslator::TargetComponentData>& comps
	    	= sttr->componentInfo();
	for ( int idx=0; idx<comps.size(); idx++ )
	{
	    int thisbps = (int)comps[idx]->datachar.nrBytes();
	    if ( icomp < 0 )
		bps += thisbps;
	    else if ( icomp == idx )
		bps = thisbps;
	}
    }

    if ( bps == 0 ) bps = 4;
    return isgood;
}


void SeisIOObjInfo::getDefKeys( BufferStringSet& bss, bool add ) const
{
    if ( !add ) bss.erase();
    if ( !isOK() ) return;

    BufferString key( ioobj_->key().buf() );
    if ( !is2D() )
	{ bss.add( key.buf() ); return; }
    else if ( isPS() )
	{ pErrMsg("2D PS not supported getting def keys"); return; }

    PtrMan<Seis2DLineSet> lset
	= new Seis2DLineSet( ioobj_->fullUserExpr(true) );
    if ( lset->nrLines() == 0 )
	return;

    BufferStringSet attrnms;
    lset->getAvailableAttributes( attrnms );
    for ( int idx=0; idx<attrnms.size(); idx++ )
	bss.add( LineKey(key.buf(),attrnms[idx]->buf()) );
}


#define mGetLineSet \
    if ( !add ) bss.erase(); \
    if ( !isOK() || !is2D() || isPS() ) return; \
 \
    PtrMan<Seis2DLineSet> lset \
	= new Seis2DLineSet( ioobj_->fullUserExpr(true) ); \
    if ( lset->nrLines() == 0 ) \
	return


void SeisIOObjInfo::getNms( BufferStringSet& bss, bool add, bool attr,
				const BinIDValueSet* bvs ) const
{
    mGetLineSet;

    BufferStringSet rejected;
    for ( int idx=0; idx<lset->nrLines(); idx++ )
    {
	const char* nm = attr ? lset->attribute(idx) : lset->lineName(idx);
	if ( bss.indexOf(nm) >= 0 )
	    continue;
	else if ( bvs )
	{
	    if ( rejected.indexOf(nm) >= 0 )
		continue;
	    if ( !lset->haveMatch(idx,*bvs) )
	    {
		rejected.add( nm );
		continue;
	    }
	}

	bss.add( nm );
    }
}


void SeisIOObjInfo::getNmsSubSel( const char* nm, BufferStringSet& bss,
				    bool add, bool l4a ) const
{

    mGetLineSet;
    if ( !nm || !*nm ) return;

    const BufferString target( nm );
    for ( int idx=0; idx<lset->nrLines(); idx++ )
    {
	const char* lnm = lset->lineName( idx );
	const char* anm = lset->attribute( idx );
	const char* requested = l4a ? anm : lnm;
	const char* listadd = l4a ? lnm : anm;

	if ( target == requested )
	    bss.addIfNew( listadd );
    }
}


BufferString SeisIOObjInfo::defKey2DispName( const char* defkey,
					     const char* ioobjnm )
{
    return LineKey::defKey2DispName( defkey, ioobjnm );
}
