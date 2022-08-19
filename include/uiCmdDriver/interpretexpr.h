#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "bufstring.h"


namespace CmdDrive
{

class CmdDriver;

mExpClass(uiCmdDriver) ExprInterpreter
{
public:
    			ExprInterpreter(const CmdDriver&);

    const char*		process(const char* exprstr,BufferString& val,
				bool isargument=false); 

    bool		isResultTrivial() const	{ return trivialresult_; }
    const char*		parsedExpr() const	{ return parsedexpr_.buf(); }
    const char*		breakPrefix() const	{ return breakprefix_.buf(); }
    bool		isParseError() const;
    const char*		errMsg() const;

protected:
    const char*		interpretSingleExpr(const char* parstr,
					    BufferString& val);
    const char*		interpretCompositeExpr(const char* parstr,
					       BufferString& val);
    void		setBreakPrefix(const char* endptr,
				       BufferString& breakprefix) const;

    const char*		exprstr_;
    bool		trivialresult_;
    BufferString	parsedexpr_;
    BufferString	breakprefix_;

    BufferString	errmsg_;
			// Start with '~'-symbol to denote parsing error

    const CmdDriver&	drv_;
};


} // namespace CmdDrive
