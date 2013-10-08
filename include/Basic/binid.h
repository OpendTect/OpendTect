#ifndef binid_h
#define binid_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		21-6-1996
 Contents:	Positions: Inline/crossline and Coordinate
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "rcol.h"
class RowCol;


/*!
\brief Positioning in a seismic survey: inline/crossline. Most functions are
identical to RowCol.
*/

mExpClass(Basic) BinID
{
public:

    typedef Pos::Index_Type	IdxType;

    inline			BinID(IdxType r,IdxType c);
    				BinID(const RowCol&);
    inline			BinID(const BinID&);
    inline			BinID();

    inline static BinID		fromInt64(od_int64);
    inline static BinID		fromInt32(int);
    inline IdxType		sqDistTo(const BinID&) const;

    const char*			getUsrStr(bool is2d=false) const;
    bool			parseUsrStr(const char*);
    inline od_int64		toInt64() const;
    bool                        isNeighborTo(const BinID&,const BinID&,
					     bool eightconnectivity=true) const;

    				// accessors
    IdxType&			inl()		{ return inl_; }
    IdxType			inl() const	{ return inl_; }
    IdxType&			crl()		{ return crl_; }
    IdxType			crl() const	{ return crl_; }

    				// aliases
    IdxType&			lineNr()	{ return inl_; }
    IdxType			lineNr() const	{ return inl_; }
    IdxType&			trcNr()		{ return crl_; }
    IdxType			trcNr() const	{ return crl_; }

    inline bool			isUdf() const	{ return *this == udf(); }
    static const BinID&		udf();

    inline bool			operator==(const BinID&) const;
    inline bool			operator!=(const BinID&) const;
    inline BinID		operator+(const BinID&) const;
    inline BinID		operator-(const BinID&) const;
    inline BinID		operator+() const;
    inline BinID		operator-() const;
    inline BinID		operator*(const BinID&) const;
    inline BinID		operator*(int) const;
    inline BinID		operator/(const BinID&) const;
    inline BinID		operator/(int) const;
    inline const BinID&		operator+=(const BinID&);
    inline const BinID&		operator-=(const BinID&);
    inline const BinID&		operator*=(const BinID&);
    inline const BinID&		operator*=(int);
    inline const BinID&		operator/=(const BinID&);

    inline IdxType&		operator[](int idx);
    inline IdxType		operator[](int idx) const;

    inline int			toInt32() const;

protected:

    IdxType			inl_;
    IdxType			crl_;

};

// These should become separate classes with their own specific functions
// at some point in time.
typedef BinID BinIDDelta;
typedef BinID BinIDAbsDelta;
typedef BinID BinIDRelDelta;
typedef BinID BinIDDeltaStep;


mImplInlineRowColFunctions(BinID, inl_, crl_);


#endif
