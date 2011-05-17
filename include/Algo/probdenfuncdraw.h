#ifndef probdenfuncdraw_h
#define probdenfuncdraw_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2010
 RCS:		$Id: probdenfuncdraw.h,v 1.1 2011-05-17 08:51:59 cvsbert Exp $
________________________________________________________________________


*/

#include "probdenfunc.h"


/* Stores one draw of a Probability Density Function. */

mClass ProbDenFuncDraw
{
public:

    			ProbDenFuncDraw( const ProbDenFunc& pdf )
			    : pdf_(pdf)		{ reset(); }
    void		reset();

    inline int		size() const		{ return vals_.size(); }
    float		value(int,bool redraw_if_used=true) const;
    inline int		useCount( int i ) const	{ return usecount_[i]; }

protected:

    const ProbDenFunc&	pdf_;
    TypeSet<float>	vals_;
    mutable TypeSet<int> usecount_;

    void		reDraw();

};


#endif
