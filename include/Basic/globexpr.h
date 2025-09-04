#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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

			GlobExpr(const char* s=nullptr,
				 OD::CaseSensitivity cs=OD::CaseSensitive);
			GlobExpr(const GlobExpr&);

    GlobExpr&		operator=(const GlobExpr&);
    bool		operator==(const GlobExpr&) const;

    void		setCaseSensitivity( OD::CaseSensitivity cs )
			{ cs_ = cs; }

    void		set(const char*);
    inline		operator const char*() const
			{ return expr_; }

    inline bool		matches( const char* t ) const
			{ return matches( expr_, t,
				 const_cast<const char*&>(errmsg_), cs_ ); }
    const char*		expressionFailMessage() const	{ return errmsg_; }
			//!< Normally null, only filled if invalid expression

    static bool		matches(const char* expression,const char* txt,
				const char*& errmsg_if_expression_is_incorrect,
				OD::CaseSensitivity cs);

    static void		validateFilterString(BufferString&);

protected:

    BufferString	expr_;
    const char*		errmsg_;
    OD::CaseSensitivity	cs_;

    static bool		starMatches(const char*,const char*,const char*&,
				    OD::CaseSensitivity);
};
