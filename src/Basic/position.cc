/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-6-1996
-*/

static const char* rcsID = "$Id: position.cc,v 1.4 2000-09-27 16:04:48 bert Exp $";

#include "survinfo.h"
#include "sets.h"
#include "separstr.h"
#include "iopar.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <values.h>

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
    if ( !str || !*str ) return NO;
    strcpy( buf, str );
    char* ptr = strchr( buf, ',' );
    if ( !ptr ) return NO;
    *ptr++ = '\0';
    x = atof( buf );
    y = atof( ptr );
    return YES;
}


void BinID::fill( char* str ) const
{
    if ( !str ) return;
    sprintf( str, "%d/%d", inl, crl );
}


bool BinID::use( const char* str )
{
    if ( !str || !*str ) return NO;
    strcpy( buf, str );
    char* ptr = strchr( buf, '/' );
    if ( !ptr ) return NO;
    *ptr++ = '\0';
    inl = atoi( buf );
    crl = atoi( ptr );
    return YES;
}


const char* BinIDExcluder::selectorType() const
{
    return "BinID";
}


bool BinIDProvider::isEqual( const BinIDProvider& bp ) const
{
    int sz = size();
    if ( sz != bp.size() ) return NO;

    for ( int idx=0; idx<sz; idx++ )
	if ( (*this)[idx] != bp[idx] ) return NO;
    return YES;
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

    BinIDRange* rg = seltyp == 0 ? new BinIDRange : new BinIDSampler;
    rg->start = SI().range().start; rg->stop = SI().range().stop;
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
	bs->step = SI().step();
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
	if ( *el4 == 'S' ) { hasso = YES; }
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


int BinIDRange::fillString( char* str ) const
{
    if ( !str ) return NO;
    FileMultiString fms;
    fms += start.inl; fms += stop.inl;
    fms += start.crl; fms += stop.crl;
    if ( stepout.inl || stepout.crl )
    {
	fms += "S";
	strcat( (char*)fms, getStringFromInt(0,stepout.inl) );
	fms += stepout.crl;
    }
    strcpy( str, fms );
    return YES;
}


int BinIDRange::extreme( bool inl, bool mini ) const
{
    return mini ? (inl ? start.inl-stepout.inl : stop.crl-stepout.crl)
		: (inl ? stop.inl+stepout.inl : stop.crl+stepout.crl);
}


int BinIDRange::excludes( const BinID& bid ) const
{
    int inlval = (!start.inl || bid.inl >= start.inl-stepout.inl)
		 && (!stop.inl || bid.inl <= stop.inl+stepout.inl) ? 0 : 2;
    int crlval = (!start.crl || bid.crl >= start.crl-stepout.crl)
		 && (!stop.crl || bid.crl <= stop.crl+stepout.crl) ? 0 : 2;
    return inlval + crlval * 256;
}


bool BinIDRange::isEq( const BinIDSelector& b ) const
{
    const BinIDRange& br = (const BinIDRange&)b;
    return br.start == start && br.stop == stop && br.stepout == stepout;
}


int BinIDRange::include( const BinID& bid, const char* )
{
    if ( bid.inl > stop.inl )  stop.inl  = bid.inl;
    if ( bid.inl < start.inl ) start.inl = bid.inl;
    if ( bid.crl > stop.crl )  stop.crl  = bid.crl;
    if ( bid.crl < start.crl ) start.crl = bid.crl;
    return YES;
}


void BinIDRange::shift( const BinID& sh )
{
    start.inl += sh.inl; stop.inl += sh.inl;
    start.crl += sh.crl; stop.crl += sh.crl;
}


int BinIDRangeProv::size() const
{
    if ( SI().step().inl < 2 && SI().step().crl < 2 )
	return (br.stop.inl - br.start.inl + 2*br.stepout.inl + 1)
	     * (br.stop.crl - br.start.crl + 2*br.stepout.crl + 1);

    BinIDSampler bs;
    ((BinIDRange&)bs) = br;
    bs.step = SI().step();
    return BinIDSamplerProv(bs).size();
}


BinID BinIDRangeProv::operator[]( int idx ) const
{
    if ( SI().step().inl < 2 && SI().step().crl < 2 )
    {
	int nrxl = br.stop.crl - br.start.crl + 2*br.stepout.crl + 1;
	int inlidx = idx / nrxl;
	int crlidx = idx - inlidx * nrxl;
	return BinID( br.start.inl - br.stepout.inl + inlidx,
		      br.start.crl - br.stepout.crl + crlidx );
    }

    BinIDSampler bs;
    ((BinIDRange&)bs) = br;
    bs.step = SI().step();
    return (BinIDSamplerProv(bs))[idx];
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


int BinIDSampler::extreme( bool inl, bool mini ) const
{
    if ( mini ) return BinIDRange::extreme(inl,mini);
    int nsteps = inl ? (stop.inl - start.inl) / (step.inl?step.inl:1)
		     : (stop.crl - start.crl) / (step.crl?step.crl:1);
    return inl	? start.inl + nsteps * step.inl + stepout.inl
		: start.crl + nsteps * step.crl + stepout.crl;
}


bool BinIDSampler::isEq( const BinIDSelector& b ) const
{
    const BinIDSampler& bs = (const BinIDSampler&)b;
    return BinIDRange::isEq(b) && bs.step == step;
}


int BinIDSampler::fillString( char* str ) const
{
    if ( !BinIDRange::fillString(str) ) return NO;

    FileMultiString fms( str );
    fms += step.inl; fms += step.crl;
    strcpy( str, fms );
    return YES;
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
    return dirSize(YES) * dirSize(NO);
}


BinID BinIDSamplerProv::operator[]( int idx ) const
{
    int nrxl = dirSize( NO );
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


class BinIDTableImpl : public TypeSet<BinID>
{
public:
	BinIDTableImpl() : TypeSet<BinID>( 0, BinID(0,0) ) {}
	BinIDTableImpl(const BinIDTableImpl&);

	TypeSet<char*>	annots;
};


BinIDTableImpl::BinIDTableImpl( const BinIDTableImpl& b )
	: TypeSet<BinID>(b)
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const char* s = b.annots[idx];
	char* news = s ? new char [ strlen(s)+1 ] : 0;
	if ( news ) strcpy( news, s );
	annots += news;
    }
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
    bool foundinl = NO; bool foundcrl = NO;
    for ( int idx=0; idx<binids.size(); idx++ )
    {
	const BinID& binid = binids[idx];
	if ( binid == bid ) return 0;

	if ( binid.inl == bid.inl ) foundinl = YES;
	if ( binid.crl == bid.crl ) foundcrl = YES;
    }

    int inlval = foundinl ? 1 : 2;   
    int crlval = foundcrl ? 1 : 2;   
    return inlval + crlval * 256;
}


int BinIDTable::extreme( bool inl, bool mini ) const
{
    if ( binids.size() == 0 ) return mini ? MAXINT : -MAXINT;
    int extr = inl ? binids[0].inl : binids[0].crl;
    for ( int idx=1; idx<binids.size(); idx++ )
    {
	if ( mini )
	{
	    if ( inl ) { if ( extr > binids[idx].inl ) extr = binids[idx].inl; }
	    else       { if ( extr > binids[idx].crl ) extr = binids[idx].crl; }
	}
	else
	{
	    if ( inl ) { if ( extr < binids[idx].inl ) extr = binids[idx].inl; }
	    else       { if ( extr < binids[idx].crl ) extr = binids[idx].crl; }
	}
    }
    return extr;
}


bool BinIDTable::isEq( const BinIDSelector& b ) const
{
    const BinIDTable& bt = (const BinIDTable&)b;
    if ( binids.size() != bt.binids.size() ) return NO;
    for ( int idx=0; idx<binids.size(); idx++ )
	if ( !bt.includes(binids[idx]) ) return NO;
    return YES;
}


int BinIDTable::includes( const BinID& bid ) const
{
    return binids.indexOf(bid) < 0 ? NO : YES;
}


int BinIDTable::include( const BinID& bid, const char* s )
{
    if ( !includes(bid) )
    {
	binids += bid;
	setAnnot( binids.size()-1, s );
    }
    return YES;
}


int BinIDTable::include( const BinIDTable& bidt )
{
    for ( int idx=0; idx<bidt.binids.size(); idx++ )
    {
	const BinID& bid = bidt.binids[idx];
	exclude( bid );
	binids += bid;
	setAnnot( binids.size()-1, bidt.binids.annots[idx] );
    }
    return YES;
}


int BinIDTable::exclude( const BinID& bid )
{
    int idx = binids.indexOf( bid );
    if ( idx < 0 ) return NO;
    binids.remove( idx ); binids.annots.remove( idx );
    return YES;
}


void BinIDTable::clear()
{
    for ( int idx=0; idx<binids.size(); idx++ )
	delete [] binids.annots[idx];
    binids.annots.erase();
    binids.erase();
}


const char* BinIDTable::annotFor( const BinID& bid ) const
{
    int sz = binids.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( binids[idx] == bid ) return annot( idx );
    }
    return 0;
}


void BinIDTable::setStepOut( const BinID& so, BinID step )
{
    // refuse if already have stepout
    if ( (!so.inl && !so.crl) || stepout.inl || stepout.crl ) return;
    stepout = so;
    BinID stepso( so.inl*step.inl, so.crl*step.crl );

    BinIDTableImpl oldbids;
    for ( int idx=0; idx<binids.size(); idx++ )
	oldbids += binids[idx];

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
    return binids.size();
}


BinID BinIDTable::operator[]( int idx ) const
{
    return binids[idx];
}


void BinIDTable::shift( const BinID& bid )
{
    for ( int idx=0; idx<binids.size(); idx++ )
	binids[idx] += bid;
}


const char* BinIDTable::annot( int idx ) const
{
    return idx < binids.annots.size() ? binids.annots[idx] : 0;
}


int BinIDTable::setAnnot( int idx, const char* s )
{
    int sz = binids.size();
    if ( idx < 0 || idx >= sz ) return NO;

    char* charptrnull = 0;
    while ( binids.annots.size() < sz )
	binids.annots += charptrnull;

    char*& mys = binids.annots[idx];
    if ( s == mys ) return YES;

    delete [] mys; mys = 0;
    if ( s )
    {
	mys = new char [ strlen(s) + 1 ];
	strcpy( mys, s );
    }
    return YES;
}
