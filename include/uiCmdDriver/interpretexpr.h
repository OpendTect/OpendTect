#ifndef interpretexpr_h
#define interpretexpr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          March 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "bufstring.h"


namespace CmdDrive
{

class CmdDriver;

mClass(uiCmdDriver) ExprInterpreter
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


}; // namespace CmdDrive


#endif

