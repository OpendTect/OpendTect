#ifndef trckey_h
#define trckey_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris/Bert/Salil
 Date:		2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "binid.h"

/*!
\brief Represents a unique trace position in one of the surveys that OpendTect
is managing.

The class is a combination of a unique survey ID and a bin position ID which is
currently implemented using a BinID (2D trace number is the crossline).

*/


mExpClass(Basic) TrcKey
{
public:

    typedef Pos::SurvID		SurvID;
    typedef IdxPair::IdxType	IdxType;

			TrcKey()			{ *this = udf(); }

			//3D
			TrcKey(const BinID&); // default 3D surv ID
			TrcKey(SurvID,const BinID&);

			//2D
			TrcKey(Pos::GeomID,Pos::TraceID);

    bool		is2D() const { return is2D(survid_); }
    static bool		is2D(SurvID);

    Pos::GeomID&	geomID();
    Pos::GeomID		geomID() const;
    static Pos::GeomID	geomID(SurvID,const BinID&);

    bool		operator==(const TrcKey&) const;

    inline bool		isUdf() const			{ return *this==udf(); }
    static const TrcKey& udf();
    static SurvID	std2DSurvID();
    static SurvID	std3DSurvID();
    static SurvID	cUndefSurvID();

    double		distTo(const TrcKey&) const;
    SurvID		survID() const			{ return survid_; }
    inline TrcKey&	setSurvID( SurvID id )
			{ survid_ = id; return *this; }

    const BinID&	position() const		{ return pos_; }
    const BinID&	binID() const			{ return pos_; }
    IdxType		inl() const			{ return pos_.row(); }
    IdxType		lineNr() const			{ return pos_.row(); }
    IdxType		crl() const			{ return pos_.col(); }
    IdxType		trcNr() const			{ return pos_.col(); }
    inline TrcKey&	setPos( const BinID& bid )
			{ pos_ = bid; return *this; }
    inline TrcKey&	setInl( IdxType nr )
			{ pos_.row() = nr; return *this; }
    inline TrcKey&	setLineNr( IdxType nr )
			{ pos_.row() = nr; return *this; }
    inline TrcKey&	setCrl( IdxType nr )
			{ pos_.col() = nr; return *this; }
    inline TrcKey&	setTrcNr( IdxType nr )
			{ pos_.col() = nr; return *this; }

    /* mDeprecated */ BinID& pos()			{ return pos_; }
			//!< Don't use. Use one of the 'set' fns.
    /* mDeprecated */ IdxType& lineNr()			{ return pos_.row(); }
    /* mDeprecated */ IdxType& trcNr()			{ return pos_.col(); }

private:

    SurvID		survid_;
    BinID		pos_;

};


#endif
