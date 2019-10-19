/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

#include "posidxpairdataset.h"
#include "testprog.h"


struct TstObj
{
		    TstObj( unsigned short f=0, unsigned int s=0 )
			: fidx_(f), sidx_(s)	    {}

    unsigned short  fidx_;
    unsigned int    sidx_;

};

struct ChckObj
{
	    ChckObj( Index_Type inl, Index_Type crl,
		     unsigned short fidx, unsigned int sidx )
		: ip_(inl,crl), obj_(fidx,sidx)	    {}
    bool    isNot( const Pos::IdxPair& ip, const TstObj& tstobj ) const
	    { return ip_ != ip || tstobj.fidx_ != obj_.fidx_
			       || tstobj.sidx_ != obj_.sidx_; }

    Pos::IdxPair	ip_;
    TstObj		obj_;
};

ObjectSet<ChckObj> chkobjs_dupl;
ObjectSet<ChckObj> chkobjs_dupl_rem;
ObjectSet<ChckObj> chkobjs_no_dupl;
ObjectSet<ChckObj> chkobjs_no_dupl_rem;

#define mAdd2Map(cobjs,inl,crl,fidx,sidx) \
    cobjs += new ChckObj( inl, crl, fidx, sidx );

static void fillExpected()
{
#define mAdd2CurMap(inl,crl,fidx,sidx) mAdd2Map(chkobjs_dupl,inl,crl,fidx,sidx)
    mAdd2CurMap(1000,2004,6,106)
    mAdd2CurMap(1000,2005,7,107)
    mAdd2CurMap(1000,2006,8,108)
    mAdd2CurMap(1000,2007,9,109)
    mAdd2CurMap(1001,2000,0,100)
    mAdd2CurMap(1001,2001,1,101)
    mAdd2CurMap(1001,2002,2,102)
    mAdd2CurMap(1001,2002,3,103)
    mAdd2CurMap(1001,2003,4,104)
    mAdd2CurMap(1001,2004,5,105)

#undef mAdd2CurMap
#define mAdd2CurMap(inl,crl,fidx,sidx) \
			mAdd2Map(chkobjs_dupl_rem,inl,crl,fidx,sidx)
    mAdd2CurMap(1000,2004,6,106)
    mAdd2CurMap(1000,2005,7,107)
    mAdd2CurMap(1000,2007,9,109)
    mAdd2CurMap(1001,2000,0,100)
    mAdd2CurMap(1001,2001,1,101)
    mAdd2CurMap(1001,2002,2,102)
    mAdd2CurMap(1001,2002,3,103)
    mAdd2CurMap(1001,2003,4,104)
    mAdd2CurMap(1001,2004,5,105)

#undef mAdd2CurMap
#define mAdd2CurMap(inl,crl,fidx,sidx) \
			mAdd2Map(chkobjs_no_dupl,inl,crl,fidx,sidx)
    mAdd2CurMap(1000,2004,6,106)
    mAdd2CurMap(1000,2005,7,107)
    mAdd2CurMap(1000,2006,8,108)
    mAdd2CurMap(1000,2007,9,109)
    mAdd2CurMap(1001,2000,0,100)
    mAdd2CurMap(1001,2001,1,101)
    mAdd2CurMap(1001,2002,2,102)
    mAdd2CurMap(1001,2003,4,104)
    mAdd2CurMap(1001,2004,5,105)

#undef mAdd2CurMap
#define mAdd2CurMap(inl,crl,fidx,sidx) \
			mAdd2Map(chkobjs_no_dupl_rem,inl,crl,fidx,sidx)
    mAdd2CurMap(1000,2004,6,106)
    mAdd2CurMap(1000,2005,7,107)
    mAdd2CurMap(1000,2007,9,109)
    mAdd2CurMap(1001,2000,0,100)
    mAdd2CurMap(1001,2001,1,101)
    mAdd2CurMap(1001,2002,2,102)
    mAdd2CurMap(1001,2003,4,104)
    mAdd2CurMap(1001,2004,5,105)
}


static const int cNrObjects = 10;

static bool chckDs( Pos::IdxPairDataSet& ds, bool isrem, const char* msg )
{
    const bool havedup = ds.allowsDuplicateIdxPairs();
    const ObjectSet<ChckObj>& chkobjs =
	   havedup ? (isrem ? chkobjs_dupl_rem : chkobjs_dupl)
		   : (isrem ? chkobjs_no_dupl_rem : chkobjs_no_dupl);

    tstStream() << msg << od_endl;
    Pos::IdxPairDataSet::SPos spos;
    Pos::IdxPair ip, previp;
    int nr = 0;
    while ( ds.next(spos) )
    {
	const TstObj* obj = static_cast<const TstObj*>( ds.get(spos,ip) );
	tstStream() << ip.first() << '/' << ip.second()
	    << "=> [" << obj->fidx_ << "," << obj->sidx_ << ']' << od_endl;

	if ( nr != 0 && !havedup && previp == ip )
	{
	    tstStream( true ) << "Duplicate in non-duplicate." << od_endl;
	    return false;
	}
	const ChckObj& chckobj = *chkobjs[nr];
	if ( chckobj.isNot(ip,*obj) )
	{
	    tstStream( true ) << "Fail at nr " << nr << od_endl;
	    return false;
	}
	nr++;
    }

    tstStream() << "All OK." << od_endl;
    return true;
}


static bool checkContents( Pos::IdxPairDataSet& ds )
{

    Pos::IdxPairDataSet::SPos spos;
    Pos::IdxPairDataSet::SPos spostorem;
    Pos::IdxPair ip, iptorem;
    Pos::IdxPairDataSet::pos_type crltorem = 1999 + 3 * cNrObjects / 4;
    while ( ds.next(spos) )
    {
	ip = ds.getIdxPair( spos );
	if ( ip.second() == crltorem )
	{
	    spostorem = spos;
	    iptorem = ip;
	}
    }

    if ( !chckDs(ds,false,BufferString( "** Before remove ", iptorem.first(),
				BufferString("/",iptorem.second())) ) )
	return false;
    ds.remove( spostorem );
    if ( !chckDs(ds, true, "** After remove:") )
	return false;

    return true;
}


static bool testAdd( Pos::IdxPairDataSet& ds, const ObjectSet<TstObj>& objs )
{
    tstStream() << "\n\nSet " << (ds.managesData() ? "[MAN] " : " ")
		<< (ds.allowsDuplicateIdxPairs() ? "with" : "without")
		<< " duplicates." << od_endl;

    ObjectSet<TstObj> useobjs;
    deepCopy( useobjs, objs );
    int inl = 1001; int crl = 2000;
    for ( int idx=0; idx<cNrObjects; idx++ )
    {
	ds.add( Pos::IdxPair(inl,crl), objs[idx] );
	if ( idx == cNrObjects/2 )
	    inl--;
	else if ( idx != cNrObjects/4 )
	    crl++;
    }
    if ( ds.managesData() )
	deepErase( useobjs );

    return checkContents( ds );
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    Pos::IdxPairDataSet* ds_viaptrs = new Pos::IdxPairDataSet( sizeof(TstObj*),
						    false, false );
    ds_viaptrs->add( Pos::IdxPair(1000,2000), new TstObj(1,2) );
    ds_viaptrs->add( Pos::IdxPair(2000,3000), new TstObj(2,3) );
    Pos::IdxPairDataSet::SPos spos;
    while ( ds_viaptrs->next(spos) )
    {
	TstObj* obj = (TstObj*)ds_viaptrs->getObj( spos );
	delete obj;
    }
    ds_viaptrs->setEmpty();
    delete ds_viaptrs;

    Pos::IdxPairDataSet ds_zeromanobj( 0, true, true );
    ds_zeromanobj.add( Pos::IdxPair(1000,2000), 0 );
    Pos::IdxPairDataSet ds_zero( 0, true, false );
    ds_zero.add( Pos::IdxPair(1000,2000), 0 );

    ObjectSet<TstObj> objs;
    for ( unsigned short idx=0; idx<cNrObjects; idx++ )
	objs += new TstObj( idx, 100+idx );
    fillExpected();

    Pos::IdxPairDataSet ds_nodup( sizeof(TstObj), false, true );
    if ( !testAdd(ds_nodup,objs) )
	return 1;
    Pos::IdxPairDataSet ds_dup( sizeof(TstObj), true, true );
    if ( !testAdd(ds_dup,objs) )
	return 1;
    Pos::IdxPairDataSet ds_nodup_noman( sizeof(TstObj), false, false );
    if ( !testAdd(ds_nodup_noman,objs) )
	return 1;
    Pos::IdxPairDataSet ds_dup_noman( sizeof(TstObj), true, false );
    if ( !testAdd(ds_dup_noman,objs) )
	return ( 1 );

    return 0;
}
