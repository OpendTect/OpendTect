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
			TrcKey(SurvID,const BinID&);
			TrcKey(SurvID,Pos::LineID,Pos::TraceID);
			TrcKey(const BinID&); // default 3D surv ID
    bool		is2D() const	{ return survid_ == std2DSurvID(); }

    			// convenience
    int&		trcNr()			{ return pos_.trcNr(); }
    int			trcNr() const		{ return pos_.trcNr(); }
    int&		lineNr()		{ return pos_.lineNr(); }
    int			lineNr() const		{ return pos_.lineNr(); }

    bool		operator ==( const TrcKey& oth ) const
    			{ return oth.survid_==survid_ && oth.pos_==pos_; }

    inline bool		isUdf() const		{ return *this==udf(); }
    static const TrcKey& udf();
    static SurvID	std2DSurvID();
    static SurvID	std3DSurvID();
    static SurvID	cUndefSurvID();

    SurvID		survID() const		{ return survid_; }
    void		setSurvID( SurvID id )	{ survid_ = id; }
    const BinID&	pos() const		{ return pos_; }
    void		setPos( BinID bid )	{ pos_ = bid; }

    SurvID		survid_;
    BinID		pos_;

};


#endif
