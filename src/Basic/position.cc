/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-6-1996
-*/

static const char* rcsID = "$Id: position.cc,v 1.28 2003-09-25 08:48:44 arend Exp $";

#include "survinfo.h"
#include "sets.h"
#include "separstr.h"
#include "iopar.h"
#include "survinfoimpl.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <ptrman.h>
#include <errh.h>

const char* BinIDSelector::sKeyseltyp   = "BinID selection";
const char* BinIDSelector::sKeyseltyps[]=
        { "No", "Range", "Regular sampling", 0 };
const char* BinIDSelector::sKeyfinl     = "First In-line";
const char* BinIDSelector::sKeylinl     = "Last In-line";
const char* BinIDSelector::sKeyfcrl     = "First Cross-line";
const char* BinIDSelector::sKeylcrl     = "Last Cross-line";
const char* BinIDSelector::sKeysoinl    = "Stepout In-line";
const char* BinIDSelector::sKeysocrl    = "Stepout Cross-line";
const char* BinIDSelector::sKeystepinl  = "Step In-line";
const char* BinIDSelector::sKeystepcrl  = "Step Cross-line";
const char* BinIDSelector::sKeywellpos	= "Well positions";
const char* BinIDSelector::sKeywellgrp	= "Well Group";
const char* BinIDSelector::sKeyreparea	= "Use representative area";
const char* BinIDSelector::sKeytab	= "Table";
const char* BinIDSelector::sKeyfname	= "File name";


double Coord::distance( const Coord& coord ) const
{
    double diffx = coord.x - x;
    double diffy = coord.y - y;
    return diffx || diffy ? sqrt( diffx*diffx + diffy*diffy ) : 0;
}


void Coord::fill( char* str ) const
{
    if ( !str ) return;
    strcpy( str, "(" ); strcat( str, getStringFromDouble(0,x) );
    strcat( str, "," ); strcat( str, getStringFromDouble(0,y) );
    strcat( str, ")" );
}


static char buf[80];

bool Coord::use( const char* str )
{
    if ( !str || !*str ) return false;

    strcpy( buf, *str == '(' ? str+1 : str );
    char* ptr = strchr( buf, ',' );
    if ( !ptr ) return false;

    *ptr++ = '\0';
    const int len = strlen(ptr);
    if ( len && ptr[len-1] == ')' ) ptr[len-1] = '\0';

    x = atof( buf );
    y = atof( ptr );
    return true;
}


void BinID::fill( char* str ) const
{
    if ( !str ) return;
    sprintf( str, "%d/%d", inl, crl );
}


bool BinID::use( const char* str )
{
    if ( !str || !*str ) return false;
    strcpy( buf, str );
    char* ptr = strchr( buf, '/' );
    if ( !ptr ) return false;
    *ptr++ = '\0';
    inl = atoi( buf );
    crl = atoi( ptr );
    return true;
}


BinID BinID::operator+( const BinID& bi ) const
{
    BinID res = *this; 
    res.inl += bi.inl; res.crl += bi.crl;
    return res; 
}


BinID BinID::operator-( const BinID& bi ) const
{
    BinID res = *this; 
    res.inl -= bi.inl; res.crl -= bi.crl;
    return res; 
}


const char* BinIDExcluder::selectorType() const
{
    return "BinID";
}


bool BinIDProvider::isEqual( const BinIDProvider& bp ) const
{
    int sz = size();
    if ( sz != bp.size() ) return false;

    for ( int idx=0; idx<sz; idx++ )
	if ( (*this)[idx] != bp[idx] ) return false;
    return true;
}


BinIDSelector* BinIDSelector::create( const IOPar& iopar )
{
    const char* res = iopar[sKeyseltyp];
    if ( !res || !*res ) return 0;

    if ( !strcmp(res,sKeyseltyps[0]) ) return 0;
    int seltyp = -1;
    if ( !strcmp(res,sKeyseltyps[1]) ) seltyp = 0;
    else if ( !strcmp(res,sKeyseltyps[2]) ) seltyp = 1;
    if ( seltyp < 0 || seltyp > 2 ) return 0;

    BinIDRange* rg = seltyp == 0 ? new BinIDRange
				 : (BinIDRange*)new BinIDSampler;
    rg->start = SI().range(false).start; rg->stop = SI().range(false).stop;
    res = iopar[sKeyfinl];
    if ( res && *res ) rg->start.inl = atoi(res);
    res = iopar[sKeyfcrl];
    if ( res && *res ) rg->start.crl = atoi(res);
    res = iopar[sKeylinl];
    if ( res && *res ) rg->stop.inl = atoi(res);
    res = iopar[sKeylcrl];
    if ( res && *res ) rg->stop.crl = atoi(res);
    if ( seltyp == 1 )
    {
	BinIDSampler* bs = (BinIDSampler*)rg;
	if ( SI().is3D() )
	    bs->step = SI3D().step(false);
	res = iopar[sKeystepinl];
	if ( res && *res ) bs->step.inl = atoi(res);
	res = iopar[sKeystepcrl];
	if ( res && *res ) bs->step.crl = atoi(res);
    }
    return rg;
}


BinIDSelector* BinIDSelector::create( const char* str )
{
    if ( !str || !*str || (*str != '-' && !isdigit(*str)) )
	return 0;

    FileMultiString fms( str );
    int sz = fms.size();
    if ( sz < 4 ) return 0;

    bool hasso = sz > 6;
    if ( sz > 4 )
    {
	const char* el4 = fms[4];
	if ( *el4 == 'S' ) { hasso = true; }
    }
    bool issamp = hasso ? sz > 6 : sz > 4;
    BinIDRange* rg = issamp ? new BinIDSampler : new BinIDRange;

    int i = 0;
    rg->start.inl = atoi( fms[i++] ); rg->stop.inl = atoi( fms[i++] );
    rg->start.crl = atoi( fms[i++] ); rg->stop.crl = atoi( fms[i++] );
    if ( hasso )
    {
	BinID so;
	so.inl = atoi( ((char*)fms[i++]) + 1 );
	so.crl = atoi( fms[i++] );
	rg->setStepOut( so );
    }
    if ( issamp )
    {
	BinIDSampler* bs = (BinIDSampler*)rg;
	bs->step.inl = atoi( fms[i++] );
	bs->step.crl = atoi( fms[i++] );
    }

    return rg;
}


void BinIDSelector::fillPar( IOPar& iopar ) const
{
    iopar.set( sKeyseltyp, sKeyseltyps[type()+1] );
    BinID so = stepOut();
    if ( so.inl || so.crl )
    {
	iopar.set( sKeysoinl, so.inl );
	iopar.set( sKeysocrl, so.crl );
    }
}

BinIDSelector* BinIDRange::getBidSel( const IOPar& iopar )
{
    BinIDSelector* bs = create( iopar );
    if ( !bs || bs->type() > 1 ) { delete bs; return 0; }

    mDynamicCastGet(BinIDRange*,br,bs)
    if ( !br ) { pErrMsg("Huh"); delete bs; return 0; }

    copyFrom( *br );
    return bs;
}


bool BinIDRange::usePar( const IOPar& iopar )
{
    PtrMan<BinIDSelector> bs = getBidSel( iopar );
    return bs ? true : false;
}


bool BinIDSampler::usePar( const IOPar& iopar )
{
    PtrMan<BinIDSelector> bsel = getBidSel( iopar );
    if ( !bsel ) return false;

    mDynamicCastGet(BinIDSampler*,bs,bsel.ptr())
    if ( !bs ) return true;
    step = bs->step;
    return true;
}


void BinIDRange::fillPar( IOPar& iopar ) const
{
    BinIDSelector::fillPar( iopar );
    iopar.set( sKeyfinl, start.inl );
    iopar.set( sKeylinl, stop.inl );
    iopar.set( sKeyfcrl, start.crl );
    iopar.set( sKeylcrl, stop.crl );
}


void BinIDSampler::fillPar( IOPar& iopar ) const
{
    BinIDRange::fillPar( iopar );
    iopar.set( sKeystepinl, step.inl );
    iopar.set( sKeystepcrl, step.crl );
}


bool BinIDRange::fillString( char* str ) const
{
    if ( !str ) return false;
    FileMultiString fms;
    fms += start.inl; fms += stop.inl;
    fms += start.crl; fms += stop.crl;
    if ( stepout.inl || stepout.crl )
    {
	fms += "S";
	strcat( fms.buf(), getStringFromInt(0,stepout.inl) );
	fms += stepout.crl;
    }
    strcpy( str, fms );
    return true;
}


BinIDRange* BinIDRange::getOuter() const
{
    return new BinIDRange( start.inl - stepout.inl, start.crl - stepout.crl,
			   stop.inl  + stepout.inl, stop.crl  + stepout.crl );
}


int BinIDRange::excludes( const BinID& bid ) const
{
    int inlval = (!start.inl || bid.inl+stepout.inl >= start.inl)
		 && (!stop.inl || bid.inl-stepout.inl <= stop.inl) ? 0 : 2;
    int crlval = (!start.crl || bid.crl+stepout.crl >= start.crl)
		 && (!stop.crl || bid.crl-stepout.crl <= stop.crl) ? 0 : 2;
    return inlval + crlval * 256;
}


bool BinIDRange::isEqBidSel( const BinIDSelector& b ) const
{
    const BinIDRange& br = (const BinIDRange&)b;
    return br.start == start && br.stop == stop && br.stepout == stepout;
}


bool BinIDRange::include( const BinID& bid, const char* )
{
    if ( bid.inl > stop.inl )  stop.inl  = bid.inl;
    if ( bid.inl < start.inl ) start.inl = bid.inl;
    if ( bid.crl > stop.crl )  stop.crl  = bid.crl;
    if ( bid.crl < start.crl ) start.crl = bid.crl;
    return true;
}


void BinIDRange::shift( const BinID& sh )
{
    start.inl += sh.inl; stop.inl += sh.inl;
    start.crl += sh.crl; stop.crl += sh.crl;
}


int BinIDRangeProv::size() const
{
    if ( SI().is3D()
      && SI3D().step(false).inl < 2 && SI3D().step(false).crl < 2 )
	return (br.stop.inl - br.start.inl + 2*br.stepout.inl + 1)
	     * (br.stop.crl - br.start.crl + 2*br.stepout.crl + 1);

    BinIDSampler bs;
    ((BinIDRange&)bs) = br;
    if ( SI().is3D() )
	bs.step = SI3D().step(false);
    return BinIDSamplerProv(bs).size();
}


BinID BinIDRangeProv::operator[]( int idx ) const
{
    if ( SI().is3D()
      && SI3D().step(false).inl < 2 && SI3D().step(false).crl < 2 )
    {
	int nrxl = br.stop.crl - br.start.crl + 2*br.stepout.crl + 1;
	int inlidx = idx / nrxl;
	int crlidx = idx - inlidx * nrxl;
	return BinID( br.start.inl - br.stepout.inl + inlidx,
		      br.start.crl - br.stepout.crl + crlidx );
    }

    BinIDSampler bs;
    ((BinIDRange&)bs) = br;
    if ( SI().is3D() )
	bs.step = SI3D().step(false);
    return (BinIDSamplerProv(bs))[idx];
}


BinIDSampler::BinIDSampler()
	: step(1,1)
{
    if ( SI().isValid() && SI().is3D() )
	step = SI3D().step(false);
}


int BinIDSampler::excludes( const BinID& bid ) const
{
    int res = BinIDRange::excludes(bid);
    if ( res ) return res;

    BinID rel( bid );
    rel.inl -= start.inl; rel.crl -= start.crl;
    int restinl = step.inl ? rel.inl % step.inl : 0;
    if ( restinl > stepout.inl ) restinl = step.inl - restinl;
    int restcrl = step.crl ? rel.crl % step.crl : 0;
    if ( restcrl > stepout.crl ) restcrl = step.crl - restcrl;

    int inlval = restinl <= stepout.inl ? 0 : 2;
    int crlval = restcrl <= stepout.crl ? 0 : 2;
    return inlval + crlval * 256;
}


bool BinIDSampler::isEqBidSel( const BinIDSelector& b ) const
{
    const BinIDSampler& bs = (const BinIDSampler&)b;
    return BinIDRange::isEq(b) && bs.step == step;
}


bool BinIDSampler::fillString( char* str ) const
{
    if ( !BinIDRange::fillString(str) ) return false;

    FileMultiString fms( str );
    fms += step.inl; fms += step.crl;
    strcpy( str, fms );
    return true;
}


int BinIDSamplerProv::ovLap( bool inl ) const
{
    int st = inl ? step.inl : step.crl;
    return st ? (inl?br.stepOut().inl:br.stepOut().crl) * 2 + 1 - st : 0;
}


int BinIDSamplerProv::dirSize( bool inl ) const
{
    int so = inl ? br.stepOut().inl : br.stepOut().crl;
    if ( ovLap(inl) >= 0 )
	return inl ? br.stop.inl-br.start.inl+1 + 2*so
		   : br.stop.crl-br.start.crl+1 + 2*so;

    int st = inl ? step.inl : step.crl;
    if ( !st ) st = 1;
    int sz = (inl ? br.stop.inl-br.start.inl
		  : br.stop.crl-br.start.crl) / st + 1;
    if ( sz < 0 ) sz = -sz;
    return sz * (so*2 + 1);
}


int BinIDSamplerProv::size() const
{
    return dirSize(true) * dirSize(false);
}


BinID BinIDSamplerProv::operator[]( int idx ) const
{
    int nrxl = dirSize( false );
    int inlidx = idx / nrxl;
    int crlidx = idx - inlidx * nrxl;

    BinID ret;
    if ( ovLap(true) >= 0 )
	ret.inl = br.start.inl - br.stepOut().inl + inlidx;
    else
    {
	int blksz = br.stepOut().inl*2 + 1;
	int nblks = inlidx / blksz;
	int inblk = inlidx - nblks * blksz;
	ret.inl = br.start.inl + nblks * step.inl + inblk - br.stepOut().inl;
    }
    if ( ovLap(false) >= 0 )
	ret.crl = br.start.crl - br.stepOut().crl + crlidx;
    else
    {
	int blksz = br.stepOut().crl*2 + 1;
	int nblks = crlidx / blksz;
	int inblk = crlidx - nblks * blksz;
	ret.crl = br.start.crl + nblks * step.crl + inblk - br.stepOut().crl;
    }

    return ret;
}


class BinIDTableImplInlData
{
public:

    class CrlAnnot
    {
    public:
			CrlAnnot( int c, const char* ann )
			: crl(c), annot(0)	{ setAnnot( ann ); }
			CrlAnnot( const CrlAnnot& ca )
			: crl(ca.crl), annot(0)	{ setAnnot( ca.annot ); }
			~CrlAnnot()		{ delete [] annot; }
        CrlAnnot&	operator =( const CrlAnnot& ca )
			{ crl = ca.crl; setAnnot( ca.annot ); return *this; }

	void		setAnnot( const char* ann )
			{
			    if ( ann == annot ) return;
			    delete [] annot; annot = 0;
			    if ( ann && *ann )
			    {
				annot = new char [ strlen(ann)+1 ];
				strcpy( annot, ann );
			    }
			}

	int		crl;
	char*		annot;
    };

			BinIDTableImplInlData( int i )
				: inl(i)	{}
			BinIDTableImplInlData( const BinIDTableImplInlData& b )
			: inl(b.inl)	{ deepAppend( data, b.data ); }
			~BinIDTableImplInlData();
    BinIDTableImplInlData& operator =( const BinIDTableImplInlData& b )
			{ inl = b.inl; deepCopy( data, b.data ); return *this; }

    int			inl;
    ObjectSet<CrlAnnot>	data;

    int			indexOf( int crl )
			{
			    for ( int idx=0; idx<data.size(); idx++ )
				if ( data[idx]->crl == crl ) return idx;
			    return -1;
			}

    void		set( int idx, int crl, const char* ann )
			{
			    if ( idx < 0 || idx >= data.size() )
				data += new CrlAnnot( crl, ann );
			    else
			    {
				CrlAnnot& ca = *data[idx];
				ca.crl = crl;
				ca.setAnnot( ann );
			    }
			}
    void		set( int crl, const char* ann )
			{
			    for ( int idx=0; idx<data.size(); idx++ )
				if ( data[idx]->crl == crl )
				    { data[idx]->setAnnot( ann ); return; }
			    data += new CrlAnnot( crl, ann );
			}
    bool		remove( int crl )
			{
			    for ( int idx=0; idx<data.size(); idx++ )
			    {
				if ( data[idx]->crl == crl )
				{
				    delete data[idx];
				    data.remove(idx);
				    return true;
				}
			    }
			    return false;
			}
    void		clear();
};


BinIDTableImplInlData::~BinIDTableImplInlData()
{
    clear();
}


void BinIDTableImplInlData::clear()
{
    deepErase( data );
}


class BinIDTableImpl : public ObjectSet< BinIDTableImplInlData >
{
public:
		BinIDTableImpl()	{}
		BinIDTableImpl( const BinIDTableImpl& b )
					{ deepAppend( *this, b ); }
		~BinIDTableImpl()	{ deepErase( *this ); }

    void	clear()			{ deepErase( *this ); }
    int		findInl(int,bool&) const;

};


int BinIDTableImpl::findInl( int inl, bool& found ) const
{
    found = false;
    int hiidx = size() - 1;
    int loidx = 0;

    if ( loidx >= hiidx )
    {
	if ( hiidx == -1 ) return -1;
	int i = (*this)[0]->inl;
	found = i == inl;
	return i <= inl ? 0 : -1;
    }

    int loinl = (*this)[loidx]->inl;
    if ( inl <= loinl )
	{ found = inl == loinl; return found ? loidx : loidx-1; }
    int hiinl = (*this)[hiidx]->inl;
    if ( inl >= hiinl )
	{ found = inl == hiinl; return hiidx; }

    while ( 1 )
    {
	int mididx = (hiidx + loidx) / 2;
	int midinl = (*this)[mididx]->inl;
	if ( midinl == inl )
	    { found = true; return mididx; }
	else if ( midinl > inl )
	{
	    if ( hiidx == mididx )
		return loidx;
	    hiidx = mididx; hiinl = midinl;
	}
	else
	{
	    if ( loidx == mididx )
		return loidx;
	    loidx = mididx; loinl = midinl;
	}
    }

    return -1;
}


BinIDTable::BinIDTable()
	: binids(*new BinIDTableImpl)
	, stepout(0,0)
{
}


BinIDTable::BinIDTable( const BinIDTable& bt )
	: binids(*new BinIDTableImpl(bt.binids))
	, stepout(bt.stepout)
	, fname(bt.fname)
{
}


BinIDTable::~BinIDTable()
{
    clear();
    delete &binids;
}


void BinIDTable::fillPar( IOPar& iopar ) const
{
    BinIDSelector::fillPar( iopar );
    iopar.set( BinIDSelector::sKeyfname, fname );
}


int BinIDTable::excludes( const BinID& bid ) const
{
    bool found;
    const int inlidx = binids.findInl( bid.inl, found );
    if ( !found ) return 2;

    return binids[inlidx]->indexOf(bid.crl) < 0 ? 512 : 0;
}


BinIDRange* BinIDTable::getOuter() const
{
    BinIDRange* ret = new BinIDRange;
    if ( binids.size() == 0 ) return ret;
    BinID cur( binids[0]->inl, binids[0]->data[0]->crl );
    ret->start = cur; ret->stop = cur;

    const int sz = binids.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	const BinIDTableImplInlData& inldat = *binids[idx];
	cur.inl = inldat.inl;
	for ( int icrl=0; icrl<inldat.data.size(); icrl++ )
	{
	    cur.crl = inldat.data[icrl]->crl;
	    ret->include( cur );
	}
    }
    return ret;
}


bool BinIDTable::isEqBidSel( const BinIDSelector& b ) const
{
    const BinIDTable& bt = (const BinIDTable&)b;
    if ( binids.size() != bt.binids.size() ) return false;
    for ( int idx=0; idx<binids.size(); idx++ )
    {
	const BinIDTableImplInlData& inldat = *binids[idx];
	if ( bt.binids[idx]->data.size() != inldat.data.size() )
	    return false;
	for ( int icrl=0; icrl<inldat.data.size(); icrl++ )
	    if ( bt.binids[idx]->indexOf( inldat.data[icrl]->crl ) < 0 )
		return false; 
    }
    return true;
}


bool BinIDTable::includes( const BinID& bid ) const
{
    return !excludes( bid );
}


bool BinIDTable::include( const BinID& bid, const char* s )
{
    bool found;
    const int inlidx = binids.findInl( bid.inl, found );
    if ( !found )
    {
	BinIDTableImplInlData* newinldat = new BinIDTableImplInlData( bid.inl );
	newinldat->set( 0 , bid.crl, s );
	binids.insertAfter( newinldat, inlidx );
    }
    else
    {
	BinIDTableImplInlData& inldat = *binids[inlidx];
	inldat.set( bid.crl, s );
    }

    return true;
}


bool BinIDTable::include( const BinIDTable& bidt )
{
    for ( int idx=0; idx<bidt.binids.size(); idx++ )
    {
	BinIDTableImplInlData& inldat = *bidt.binids[idx];
	for ( int icrl=0; icrl<inldat.data.size(); icrl++ )
	    include( BinID(inldat.inl,inldat.data[icrl]->crl),
		     inldat.data[icrl]->annot );
    }
    return true;
}


bool BinIDTable::exclude( const BinID& bid )
{
    bool found;
    const int inlidx = binids.findInl( bid.inl, found );
    if ( !found ) return false;

    return binids[inlidx]->remove( bid.crl );
}


void BinIDTable::clear()
{
    binids.clear();
}


const char* BinIDTable::annotFor( const BinID& bid ) const
{
    bool found = false;
    const int inlidx = binids.findInl( bid.inl, found );
    if ( !found ) return 0;
    BinIDTableImplInlData& inldat = *binids[inlidx];
    int crlidx = inldat.indexOf( bid.crl );
    return crlidx < 0 ? 0 : inldat.data[crlidx]->annot;
}


void BinIDTable::setStepOut( const BinID& so, BinID step )
{
    // refuse if already have stepout
    if ( (!so.inl && !so.crl) || stepout.inl || stepout.crl ) return;
    stepout = so;
    BinID stepso( so.inl*step.inl, so.crl*step.crl );

    TypeSet<BinID> oldbids;
    for ( int idx=0; idx<binids.size(); idx++ )
    {
	BinIDTableImplInlData& inldat = *binids[idx];
	for ( int icrl=0; icrl<inldat.data.size(); icrl++ )
	    oldbids += BinID(inldat.inl,inldat.data[icrl]->crl);
    }

    for ( int idx=0; idx<oldbids.size(); idx++ )
    {
	BinID bid = oldbids[idx];
        for ( int iinl=-stepso.inl; iinl<=stepso.inl; iinl+=step.inl )
        {   
            for ( int icrl=-stepso.crl; icrl<=stepso.crl; icrl+=step.crl )
	    {
		if ( iinl || icrl )
		    include( BinID(bid.inl+iinl,bid.crl+icrl) );
	    }
        }
    }
}


int BinIDTable::size() const
{
    int sz = 0;
    for ( int idx=0; idx<binids.size(); idx++ )
	sz += binids[idx]->data.size();
    return sz;
}


BinID BinIDTable::operator[]( int ifind ) const
{
    int sz = 0;
    for ( int idx=0; idx<binids.size(); idx++ )
    {
	const BinIDTableImplInlData& inldat = *binids[idx];
	sz += inldat.data.size();
	if ( sz > ifind )
	{
	    sz -= inldat.data.size();
	    return BinID( inldat.inl, inldat.data[ifind-sz]->crl );
	}
    }
    return BinID(0,0);
}


void BinIDTable::shift( const BinID& bid )
{
    for ( int idx=0; idx<binids.size(); idx++ )
    {
	BinIDTableImplInlData& inldat = *binids[idx];
	inldat.inl += bid.inl;
	for ( int icrl=0; icrl<inldat.data.size(); idx++ )
	    inldat.data[icrl]->crl += bid.crl;
    }
}


const char* BinIDTable::annot( int ifind ) const
{
    int sz = 0;
    for ( int idx=0; idx<binids.size(); idx++ )
    {
	const BinIDTableImplInlData& inldat = *binids[idx];
	sz += inldat.data.size();
	if ( sz > ifind )
	{
	    sz -= inldat.data.size();
	    return inldat.data[ ifind - sz ]->annot;
	}
    }
    return 0;
}


bool BinIDTable::setAnnot( int ifind, const char* s )
{
    int sz = 0;
    for ( int idx=0; idx<binids.size(); idx++ )
    {
	BinIDTableImplInlData& inldat = *binids[idx];
	sz += inldat.data.size();
	if ( sz > ifind )
	{
	    sz -= inldat.data.size();
	    inldat.set( ifind - sz, inldat.data[ifind-sz]->crl, s );
	    return true;
	}
    }
    return false;
}


bool Coord3::operator==( const Coord3& b ) const
{ 
    const float dx = x-b.x; const float dy = y-b.y; const float dz = z-b.z;
    return mIS_ZERO(dx) && mIS_ZERO(dy) && mIS_ZERO(dz);
}


bool Coord3::operator!=( const Coord3& b ) const
{
    return !(b==*this);
}


bool Coord3::isDefined() const
{
    return !mIsUndefined(x) && !mIsUndefined(y) && !mIsUndefined(z);
}


Coord3 Coord3::operator+( const Coord3& p ) const
{
    return Coord3( x+p.x, y+p.y, z+p.z );
}


Coord3 Coord3::operator-( const Coord3& p ) const
{
    return Coord3( x-p.x, y-p.y, z-p.z );
}


Coord3& Coord3::operator+=( const Coord3& p )
{
    x += p.x; y += p.y; z += p.z;
    return *this;
}


Coord3& Coord3::operator-=( const Coord3& p )
{
    x -= p.x; y -= p.y; z -= p.z;
    return *this;
}


double Coord3::distance( const Coord3& b ) const
{
    double dx = x-b.x, dy = y-b.y, dz = z-b.z;
    return sqrt( dx*dx + dy*dy + dz*dz );
}
