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

    typedef Pos::SurvID	SurvID;

			TrcKey()		{ *this = udf(); }

			//3D
			TrcKey(const BinID&); // default 3D surv ID
			TrcKey(SurvID,const BinID&);

			//2D
			TrcKey(Pos::GeomID,Pos::TraceID);

    bool		is2D() const { return is2D(survid_); }
    static bool		is2D(SurvID);

    Pos::TraceID&	trcNr();
    Pos::TraceID	trcNr() const;
    Pos::LineID&	lineNr();
			//!<Not valid for 2D. Returns bogus
    Pos::LineID		lineNr() const;
			//!<Not valid for 2D. Returns bogus

    Pos::GeomID&	geomID();
    Pos::GeomID		geomID() const;
    static Pos::GeomID	geomID(SurvID,const BinID&);

    bool		operator==(const TrcKey&) const;

    inline bool		isUdf() const		{ return *this==udf(); }
    static const TrcKey& udf();
    static SurvID	std2DSurvID();
    static SurvID	std3DSurvID();
    static SurvID	cUndefSurvID();

    SurvID		survID() const		{ return survid_; }
    void		setSurvID( SurvID id )	{ survid_ = id; }
    const BinID&	pos() const		{ return pos_; }
    void		setPos( const BinID& bid )	{ pos_ = bid; }
    double		distTo(const TrcKey&) const;

private:
    SurvID		survid_;
    BinID		pos_;

};


#endif

