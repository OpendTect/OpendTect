/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-6-1996
-*/

static const char* rcsID = "$Id: position.cc,v 1.1.1.2 1999-09-16 09:32:35 arend Exp $";

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
const char* BinIDSelector::sKeydist	= "Maximum distance";
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
    strcpy( str, "(" );
    strcat( str, getStringFromDouble("%lg",x) );
    strcat( str, "," );
    strcat( str, getStringFromDouble("%lg",y) );
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


bool BinIDProvider::isEqual( const BinIDProvider& bp ) const
{
    int sz = size();
    if ( sz != bp.size() ) return NO;

    for ( int idx=0; idx<sz; idx++ )
	if ( (*this)[idx] != bp[idx] ) return NO;
    return YES;
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
	strcat( (char*)fms, getStringFromInt("%d",stepout.inl) );
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
    int inlok = (!start.inl || bid.inl >= start.inl-stepout.inl)
		&& (!stop.inl || bid.inl <= stop.inl+stepout.inl);
    int crlok = (!start.crl || bid.crl >= start.crl-stepout.crl)
		&& (!stop.crl || bid.crl <= stop.crl+stepout.crl);
    return inlok && crlok ? 0 : (inlok ? 2 : (crlok ? 1 : 3));
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


int BinIDRange::size() const
{
    if ( SI().step().inl < 2 && SI().step().crl < 2 )
	return (stop.inl - start.inl + 2*stepout.inl + 1)
	     * (stop.crl - start.crl + 2*stepout.crl + 1);

    BinIDSampler bs;
    ((BinIDRange&)bs) = *this;
    bs.step = SI().step();
    return bs.size();
}


BinID BinIDRange::operator[]( int idx ) const
{
    if ( SI().step().inl < 2 && SI().step().crl < 2 )
    {
	int nrxl = stop.crl - start.crl + 2*stepout.crl + 1;
	int inlidx = idx / nrxl;
	int crlidx = idx - inlidx * nrxl;
	return BinID( start.inl - stepout.inl + inlidx,
		      start.crl - stepout.crl + crlidx );
    }

    BinIDSampler bs;
    ((BinIDRange&)bs) = *this;
    bs.step = SI().step();
    return bs[idx];
}


void BinIDRange::shift( const BinID& sh )
{
    start.inl += sh.inl; stop.inl += sh.inl;
    start.crl += sh.crl; stop.crl += sh.crl;
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
    bool inlok = restinl <= stepout.inl;
    bool crlok = restcrl <= stepout.crl;

    return inlok && crlok ? 0 : (inlok ? 2 : (crlok ? 1 : 3));
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


int BinIDSampler::ovLap( bool inl ) const
{
    int st = inl ? step.inl : step.crl;
    return st ? (inl?stepout.inl:stepout.crl) * 2 + 1 - st : 0;
}


int BinIDSampler::dirSize( bool inl ) const
{
    int so = inl ? stepout.inl : stepout.crl;
    if ( ovLap(inl) >= 0 )
	return inl ? stop.inl-start.inl+1 + 2*so : stop.crl-start.crl+1 + 2*so;

    int st = inl ? step.inl : step.crl;
    if ( !st ) st = 1;
    int sz = (inl ? stop.inl-start.inl : stop.crl-start.crl) / st + 1;
    if ( sz < 0 ) sz = -sz;
    return sz * (so*2 + 1);
}


int BinIDSampler::size() const
{
    return dirSize(YES) * dirSize(NO);
}


BinID BinIDSampler::operator[]( int idx ) const
{
    int nrxl = dirSize( NO );
    int inlidx = idx / nrxl;
    int crlidx = idx - inlidx * nrxl;

    BinID ret;
    if ( ovLap(YES) >= 0 )
	ret.inl = start.inl - stepout.inl + inlidx;
    else
    {
	int blksz = stepout.inl*2 + 1;
	int nblks = inlidx / blksz;
	int inblk = inlidx - nblks * blksz;
	ret.inl = start.inl + nblks * step.inl + inblk - stepout.inl;
    }
    if ( ovLap(NO) >= 0 )
	ret.crl = start.crl - stepout.crl + crlidx;
    else
    {
	int blksz = stepout.crl*2 + 1;
	int nblks = crlidx / blksz;
	int inblk = crlidx - nblks * blksz;
	ret.crl = start.crl + nblks * step.crl + inblk - stepout.crl;
    }

    return ret;
}


int BinIDSampler::fillString( char* str ) const
{
    if ( !BinIDRange::fillString(str) ) return NO;

    FileMultiString fms( str );
    fms += step.inl; fms += step.crl;
    strcpy( str, fms );
    return YES;
}


class BinIDTableImpl : public TypeSet<BinID>
{
public:
	BinIDTableImpl() : TypeSet<BinID>( 0, BinID(0,0) ) {}

	TypeSet<char*>	annots;
};


BinIDTable::BinIDTable( double d )
	: dist(d)
	, binids(*new BinIDTableImpl)
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


static char* charptrnull = 0;

BinIDTable* BinIDTable::clone() const
{
    BinIDTable* tab = new BinIDTable( dist );
    for ( int idx=0; idx<binids.size(); idx++ )
    {
	tab->binids += binids[idx];
	tab->binids.annots += charptrnull;
	tab->setAnnot( idx, binids.annots[idx] );
    }

    return tab;
}


int BinIDTable::excludes( const BinID& bid ) const
{
    bool foundinl = NO; bool foundcrl = NO;
    bool seenmatch = NO;
    for ( int idx=0; idx<binids.size(); idx++ )
    {
	const BinID& binid = binids[idx];
	if ( binid == bid ) return 0;
	if ( binid.inl == bid.inl ) foundinl = YES;
	if ( binid.crl == bid.crl ) foundcrl = YES;
	if ( foundinl && foundcrl ) seenmatch = YES;
    }
    if ( !foundinl && !foundcrl ) return 3;
    return foundinl ? (foundcrl ? (seenmatch?7:6) : 2 ) : 1;
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
    if ( size() != bt.size() ) return NO;
    for ( int idx=0; idx<size(); idx++ )
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


void BinIDTable::clear()
{
    for ( int idx=0; idx<binids.size(); idx++ )
	delete [] binids.annots[idx];
    binids.annots.erase();
    binids.erase();
}


const char* BinIDTable::annotFor( const BinID& bid ) const
{
    int sz = size();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( (*this)[idx] == bid ) return annot( idx );
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


const char* BinIDTable::annot( int idx ) const
{
    return idx < binids.annots.size() ? binids.annots[idx] : 0;
}


int BinIDTable::setAnnot( int idx, const char* s )
{
    int sz = size();
    if ( idx < 0 || idx >= sz ) return NO;
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
