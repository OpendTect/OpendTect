#ifndef binidsorting_h
#define binidsorting_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Dec 2006
 RCS:		$Id: binidsorting.h,v 1.5 2010/07/12 22:52:41 cvskris Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "bufstring.h"


/*!\brief BinID sorting parameters

  Note that in 2D, inl == line number, crl == trace number
 
 */

mClass BinIDSorting
{
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
    static const char*	description(bool is2d,bool inlsorted,bool inlupward,
	    			    bool crlupward);

protected:

    friend class	BinIDSortingAnalyser;

    			BinIDSorting( bool is2d, int st )
			    : is2d_(is2d), state_(st)	{}

    int			state_;
    bool		is2d_;

};


mClass BinIDSortingAnalyser
{
public:
    			BinIDSortingAnalyser(bool is2d);

    bool		add(const BinID&);
    			//!< returns whether analysis phase is finished
    			//!< Check errMsg() to see whether a vaild sorting found
    BinIDSorting	getSorting() const;
    			//!< Can be used after add() returns true
    const char*		errMsg() const		{ return errmsg_.str(); }

protected:

    BinID		prev_;
    bool		is2d_;
    bool		st_[8];
    BufferString	errmsg_;

};


#endif
