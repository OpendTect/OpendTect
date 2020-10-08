#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		15-1-2001
________________________________________________________________________

*/

#include "basicmod.h"
#include "bufstring.h"

/*!
\brief Glob-expression matching like UNIX shells

  Expressions may have '*', '?' and [] (e.g. [a-eA-E]) constructions. Also the
  inverted selection with '^' or '!' supported (e.g. [^x-z] or [!akPZ]). Escape
  the special chars with '\';
*/

mExpClass(Basic) GlobExpr
{
public:

			GlobExpr( const char* s=nullptr, mODStringDefSens )
			    : expr_(""), errmsg_(nullptr), ci_(cs==CaseInsensitive)
				{ set(s); }
			GlobExpr( const GlobExpr& ge )
			    : expr_(ge.expr_), errmsg_(nullptr), ci_(ge.ci_) {}
    GlobExpr&		operator=( const GlobExpr& ge )
				{ expr_ = ge.expr_; errmsg_ = nullptr; ci_ = ge.ci_;
				  return *this; }
    bool		operator==( const GlobExpr& ge ) const
				{ return expr_ == ge.expr_ && ci_ == ge.ci_; }

    void		setCaseInsensitive( bool yn=true )	{ ci_ = yn; }

    void		set(const char*);
    inline		operator const char*() const
			{ return expr_; }

    inline bool		matches( const char* t ) const
				{ return matches( expr_, t,
				  const_cast<const char*&>(errmsg_), ci_ ); }
    const char*		expressionFailMessage() const	{ return errmsg_; }
				//!< only non-null if invalid expression

    static bool		matches(const char* expression,const char* txt,
				const char*& errmsg_if_expression_is_incorrect,
				bool caseinsens);

protected:

    BufferString	expr_;
    const char*		errmsg_;
    bool		ci_;

    static bool		starMatches(const char*,const char*,const char*&,bool);

};
