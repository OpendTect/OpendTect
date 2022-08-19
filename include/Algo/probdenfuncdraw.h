#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "probdenfunc.h"


/*!
\brief Stores one draw of a probability density function.
*/

mExpClass(Algo) ProbDenFuncDraw
{
public:

    			ProbDenFuncDraw( const ProbDenFunc& p )
			    : pdf_(p)		{ reset(); }
			~ProbDenFuncDraw();
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
