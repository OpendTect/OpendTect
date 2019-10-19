#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2019
________________________________________________________________________

-*/

#include "geomid.h"
#include "binid.h"
#include "uistring.h"


/*! \brief uniquely identifies a trace position on a 2D geometry.
Did not inherit from Pos::IdxPair to make it harder to interchange BinID and
Bin2D */


mExpClass(Basic) Bin2D
{
public:

    mUseType( Pos,		IdxPair );
    mUseType( Pos,		GeomID );
    typedef IdxPair::pos_type	trcnr_type;

    inline		Bin2D() : trcnr_(mUdf(trcnr_type))		{}
    explicit inline	Bin2D( GeomID gid, trcnr_type tnr=mUdf(trcnr_type) )
			    : geomid_(gid), trcnr_(tnr)			{}
    explicit inline	Bin2D( const IdxPair& ip )
			    : geomid_(ip.first()), trcnr_(ip.second())	{}
			mImplSimpleEqOpers2Memb(Bin2D,geomid_,trcnr_)

    GeomID&		geomID()	{ return geomid_; }
    GeomID		geomID() const	{ return geomid_; }
    trcnr_type&		trcNr()		{ return trcnr_; }
    trcnr_type		trcNr() const	{ return trcnr_; }
    GeomID::IDType	lineNr() const	{ return geomid_.lineNr(); }
    Coord		coord() const;

    static Bin2D	first(GeomID);	//!< start of the line
    static Bin2D	last(GeomID);	//!< end of the line

    IdxPair		idxPair() const	{ return IdxPair(lineNr(),trcNr()); }
    void		encodeIn(BinID&) const;
    void		decodeFrom(const BinID&);
    inline static Bin2D	decode( const BinID& bid )
			{ Bin2D ret; ret.decodeFrom(bid); return ret; }

    const char*		toString() const;
    BufferString	lineName() const
			{ return BufferString(geomid_.name()); }
    BufferString	usrDispStr() const;
    bool		fromString(const char*);

    static Bin2D	udf()	{ return Bin2D(); }

protected:

    GeomID	geomid_;
    trcnr_type	trcnr_;

};
