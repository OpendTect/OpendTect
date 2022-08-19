#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "binid.h"
#include "bufstring.h"
#include "uistring.h"


/*!
\brief BinID sorting parameters

  Note that in 2D, inl == line number, crl == trace number.
*/

mExpClass(Algo) BinIDSorting
{ mODTextTranslationClass(BinIDSorting)
public:
    			BinIDSorting( bool is2d )
			    : is2d_(is2d), state_(0)	{}

    bool		isValid(const BinID& prev,const BinID& cur) const;
    const char*		description() const;

    bool		inlSorted() const
			{ return state_ < 4; }
    bool		inlUpward() const //!< ignored in 2D
			{ return (state_ % 2) < 1; }
    bool		crlUpward() const
			{ return (state_ % 4) < 2; }

    void		set(bool inl,bool inlupw,bool crlupw);
    			//!< In 2D, 'inl' is ignored and inlupw irrelevant

    static bool		isValid(bool is2d,const BinID& prev,const BinID& cur,
	    			bool inlsorted,bool inlupward,bool crlupward);
    static BufferString	description(bool is2d,bool inlsorted,bool inlupward,
	    			    bool crlupward);

protected:

    friend class	BinIDSortingAnalyser;

    			BinIDSorting( bool is2d, int st )
			    : is2d_(is2d), state_(st)	{}

    int			state_;
    bool		is2d_;

};


/*!
\brief Analyses whether input BinIDs are sorted.
*/

mExpClass(Algo) BinIDSortingAnalyser
{ mODTextTranslationClass(BinIDSortingAnalyser)
public:
    			BinIDSortingAnalyser(bool is2d);

    bool		add(const BinID&);
    			//!< returns whether analysis phase is finished
    			//!< Check errMsg() to see whether a valid sorting found
    BinIDSorting	getSorting() const;
    			//!< Can be used after add() returns true
    uiString		errMsg()		{ return errmsg_; }

protected:

    BinID		prev_;
    bool		is2d_;
    bool		st_[8];
    uiString	        errmsg_;

};
