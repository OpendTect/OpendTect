#ifndef globexpr_h
#define globexpr_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		15-1-2001
 RCS:		$Id$
________________________________________________________________________

*/

#include "basicmod.h"
#include "general.h"

/*!
\brief Glob-expression matching like UNIX shells

  Expressions may have '*', '?' and [] (e.g. [a-eA-E]) constructions. Also the
  inverted selection with '^' or '!' supported (e.g. [^x-z] or [!akPZ]). Escape
  the special chars with '\';
*/

mExpClass(Basic) GlobExpr
{
public:

			GlobExpr( const char* s = 0, bool casesens=true )
			: expr_(""), errmsg_(0), ci_(!casesens)	{ set(s); }
			GlobExpr( const GlobExpr& ge )
			: expr_(ge.expr_), errmsg_(0), ci_(ge.ci_) {}
    GlobExpr&		operator=( const GlobExpr& ge )
			{ expr_ = ge.expr_; errmsg_ = 0; ci_ = ge.ci_;
			  return *this; }
    bool		operator==( const GlobExpr& ge ) const
			{ return expr_ == ge.expr_ && ci_ == ge.ci_; }

    void		setCaseInSensitive( bool yn=true )	{ ci_ = yn; }

    void		set(const char*);
    inline		operator const char*() const
			{ return (const char*)expr_; }

    inline bool		matches( const char* t ) const
			{ return matches( expr_, t,
				 const_cast<const char*&>(errmsg_), ci_ ); }
    const char*		expressionFailMessage() const	{ return errmsg_; }
			//!< Normally null, only filled if invalid expression

    static bool		matches(const char* expression,const char* txt,
				const char*& errmsg_if_expression_is_incorrect,
				bool caseinsens);

protected:

    BufferString	expr_;
    const char*		errmsg_;
    bool		ci_;

    static bool		starMatches(const char*,const char*,const char*&,bool);
};


#endif

