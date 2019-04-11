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


/*! \brief uniquely identifies a trace position on a 2D geometry */


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
			    : geomid_(ip.first), trcnr_(ip.second)	{}

    GeomID&		geomID()	    { return geomid_; }
    GeomID		geomID() const	    { return geomid_; }
    trcnr_type&		trcNr()		    { return trcnr_; }
    trcnr_type		trcNr() const	    { return trcnr_; }

    static Bin2D	first(GeomID);
    static Bin2D	last(GeomID);
    void		encodeIn(BinID&) const;
    void		decodeFrom(const BinID&);

    const char*		toString() const;
    BufferString	usrDispStr() const;
    bool		fromString(const char*);

    static Bin2D	udf()	{ return Bin2D(); }

protected:

    GeomID	geomid_;
    trcnr_type	trcnr_;

};
