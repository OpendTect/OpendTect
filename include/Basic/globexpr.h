#ifndef globexpr_h
#define globexpr_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		15-1-2001
 RCS:		$Id: globexpr.h,v 1.2 2003-11-07 12:21:50 bert Exp $
________________________________________________________________________

*/

#include <general.h>

/*!\brief Glob-expression matching like UNIX shells

Expressions may have '*', '?' and [] (e.g. [a-eA-E]) constructions. Also the
inverted selection with '^' or '!' supported (e.g. [^x-z] or [!akPZ]). Escape
the special chars with '\';

*/


class GlobExpr
{
public:

			GlobExpr( const char* s = 0 )
			: expr_(""), errmsg_(0)			{ set(s); }
			GlobExpr( const GlobExpr& ge )
			: expr_(ge.expr_), errmsg_(0)		{}
    GlobExpr&		operator=( const GlobExpr& ge )
			{ expr_ = ge.expr_; errmsg_ = 0; return *this; }
    bool		operator==( const GlobExpr& ge ) const
			{ return expr_ == ge.expr_; }
			
    void		set(const char*);
    inline		operator const char*() const
			{ return (const char*)expr_; }

    inline bool		matches( const char* t ) const
			{ return matches( expr_, t,
				 const_cast<const char*&>(errmsg_) ); }
    const char*		expressionFailMessage() const	{ return errmsg_; }
			//!< Normally null, only filled if invalid expression

    static bool		matches(const char* expression,const char* txt,
				const char*& errmsg_if_expression_is_incorrect);

protected:

    BufferString	expr_;
    const char*		errmsg_;

    static bool		starMatches(const char*,const char*,const char*&);
};


#endif
