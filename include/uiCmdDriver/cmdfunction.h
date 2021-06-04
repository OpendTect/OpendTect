#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          April 2011
________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "bufstringset.h"
#include "factory.h"


class uiMainWin;

namespace CmdDrive
{

class CmdDriver;
class WildcardManager;

mExpClass(uiCmdDriver) Function
{
public:

    mDefineFactory1ParamInClass( Function, const CmdDriver&, factory );
    static void		initStandardFunctions();
    static BufferString	factoryKey(const char* name);

			Function(const CmdDriver& cmddrv)
			    : drv_(cmddrv)
			{}
    virtual		~Function()					{}

    virtual const char*	name() const					=0;
    virtual bool	eval(const BufferStringSet& args,
			     BufferString& res) const			=0;


protected:

    static BufferString	createFactoryKey(const char* keyword);

    const CmdDriver&	drv_;

    bool		openQDlg() const;
    const uiMainWin*	curWin() const;
    const uiMainWin*	applWin() const;

    const WildcardManager& wildcardMan() const;
};


#define mStartDeclFunClassNoEval(funkey,parentclass) \
\
mExpClass(uiCmdDriver) funkey##Func : public parentclass \
{ \
public: \
			funkey##Func(const CmdDriver& cmddrv) \
			    : parentclass(cmddrv) \
			{ \
			    name_ = keyWord(); \
			    *name_.getCStr() = mCast(char,tolower(*name_.buf())); \
			} \
\
    static const char*	keyWord()			{ return #funkey; } \
    virtual const char* name() const			{ return name_; } \
protected: \
    BufferString	name_;


#define mStartDeclFunClass(funkey,parentclass) \
\
    mStartDeclFunClassNoEval(funkey,parentclass) \
public: \
    virtual bool        eval(const BufferStringSet& args, \
			     BufferString& res) const; \
\
    static Function*	createInstance(const CmdDriver& cmddrv) \
			{ return new funkey##Func(cmddrv); } \
    static void		initClass() \
			{ factory().addCreator( createInstance, \
						createFactoryKey(keyWord()) ); }

#define mEndDeclFunClass \
};


mStartDeclFunClass( Abs,	Function )	mEndDeclFunClass
mStartDeclFunClass( Asin,	Function )	mEndDeclFunClass
mStartDeclFunClass( Acos,	Function )	mEndDeclFunClass
mStartDeclFunClass( Atan,	Function )	mEndDeclFunClass
mStartDeclFunClass( Atan2,	Function )	mEndDeclFunClass
mStartDeclFunClass( Ceil,	Function )	mEndDeclFunClass
mStartDeclFunClass( Cos,	Function )	mEndDeclFunClass
mStartDeclFunClass( Exp,	Function )	mEndDeclFunClass
mStartDeclFunClass( Floor,	Function )	mEndDeclFunClass
mStartDeclFunClass( Ln,		Function )	mEndDeclFunClass
mStartDeclFunClass( Log,	Function )	mEndDeclFunClass
mStartDeclFunClass( Rand,	Function )	mEndDeclFunClass
mStartDeclFunClass( RandG,	Function )	mEndDeclFunClass
mStartDeclFunClass( Round,	Function )	mEndDeclFunClass
mStartDeclFunClass( Sgn,	Function )	mEndDeclFunClass
mStartDeclFunClass( Sin,	Function )	mEndDeclFunClass
mStartDeclFunClass( Sqrt,	Function )	mEndDeclFunClass
mStartDeclFunClass( Tan,	Function )	mEndDeclFunClass
mStartDeclFunClass( Trunc,	Function )	mEndDeclFunClass

mStartDeclFunClass( Avg,	Function )	mEndDeclFunClass
mStartDeclFunClass( Max,	Function )	mEndDeclFunClass
mStartDeclFunClass( Med,	Function )	mEndDeclFunClass
mStartDeclFunClass( Min,	Function )	mEndDeclFunClass
mStartDeclFunClass( Sum,	Function )	mEndDeclFunClass
mStartDeclFunClass( Var,	Function )	mEndDeclFunClass

mStartDeclFunClass( IsAlNum,	Function )	mEndDeclFunClass
mStartDeclFunClass( IsAlpha,	Function )	mEndDeclFunClass
mStartDeclFunClass( IsDigit,	Function )	mEndDeclFunClass
mStartDeclFunClass( IsLower,	Function )	mEndDeclFunClass
mStartDeclFunClass( IsSpace,	Function )	mEndDeclFunClass
mStartDeclFunClass( IsUpper,	Function )	mEndDeclFunClass

mStartDeclFunClass( ToLower,	Function )	mEndDeclFunClass
mStartDeclFunClass( ToUpper,	Function )	mEndDeclFunClass

mStartDeclFunClass( IsNumber,	Function )	mEndDeclFunClass
mStartDeclFunClass( IsInteger,	Function )	mEndDeclFunClass

mStartDeclFunClass( StrCat,	Function )	mEndDeclFunClass
mStartDeclFunClass( StrLen,	Function )	mEndDeclFunClass
mStartDeclFunClass( StrSel,	Function )	mEndDeclFunClass

mStartDeclFunClass( SepStrCat,	Function )	mEndDeclFunClass
mStartDeclFunClass( SepStrLen,	Function )	mEndDeclFunClass
mStartDeclFunClass( SepStrSel,	Function )	mEndDeclFunClass

mStartDeclFunClass( Wildcard,		Function )	mEndDeclFunClass
mStartDeclFunClass( WildcardStr,	Function )	mEndDeclFunClass

mStartDeclFunClass( CurWindow,	Function )	mEndDeclFunClass

}; // namespace CmdDrive


