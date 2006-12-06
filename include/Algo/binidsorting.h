#ifndef binidsorting_h
#define binidsorting_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Dec 2006
 RCS:		$Id: binidsorting.h,v 1.1 2006-12-06 11:59:53 cvsbert Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "bufstring.h"


/*!\brief BinID sorting parameters */

class BinIDSorting
{
public:
    			BinIDSorting() : state_(0)	{}

    bool		isValid(const BinID& prev,const BinID& cur) const;
    const char*		description() const;

    void		set(bool inl,bool inlupw,bool crlupw);
    bool		inlSorted() const
			{ return state_ < 4; }
    bool		inlUpward() const
			{ return (state_ % 2) < 1; }
    bool		crlUpward() const
			{ return (state_ % 4) < 2; }

    static bool		isValid(const BinID& prev,const BinID& cur,
	    			bool inlsorted,bool inlupward,bool crlupward);
    static const char*	description(bool inlsorted,bool inlupward,
	    			    bool crlupward);

protected:

    friend class	BinIDSortingAnalyser;

    			BinIDSorting( int st ) : state_(st)	{}

    int			state_;

};


class BinIDSortingAnalyser
{
public:
    			BinIDSortingAnalyser(BinID bid=
						BinID(mUdf(int),mUdf(int)));

    bool		add(const BinID&);
    			//!< returns whether analysis phase is finished
    			//!< Check errMsg() to see whether a vaild sorting found
    BinIDSorting	getSorting() const;
    			//!< Can be used after add() returns true
    const char*		errMsg() const		{ return errmsg_; }

protected:

    BufferString	errmsg_;
    BinID		prev_;
    bool		st_[8];

};


#endif
