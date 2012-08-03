#ifndef probdenfuncdraw_h
#define probdenfuncdraw_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2010
 RCS:		$Id: probdenfuncdraw.h,v 1.3 2012-08-03 13:00:05 cvskris Exp $
________________________________________________________________________


*/

#include "algomod.h"
#include "probdenfunc.h"


/* Stores one draw of a Probability Density Function. */

mClass(Algo) ProbDenFuncDraw
{
public:

    			ProbDenFuncDraw( const ProbDenFunc& p )
			    : pdf_(p)		{ reset(); }
    void		reset();

    inline int		size() const		{ return vals_.size(); }
    float		value(int,bool redraw_if_used=true) const;
    inline int		useCount( int i ) const	{ return usecount_[i]; }

    const ProbDenFunc&	pdf() const		{ return pdf_; }
    const TypeSet<float>& vals() const		{ return vals_; }

protected:

    const ProbDenFunc&	pdf_;
    TypeSet<float>	vals_;
    mutable TypeSet<int> usecount_;

    void		reDraw();

};


#endif

