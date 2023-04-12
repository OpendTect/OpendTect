#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "commandlineparser.h"
#include "genc.h"
#include "iopar.h"
#include "odruncontext.h"
#include "od_iosfwd.h"
class CommandLineParser;
class Timer;
class uiRetVal;
namespace OD { namespace JSON { class Array; class Object; } }

mExpClass(General) ServerProgTool : public CallBacker
{
public:

    typedef OD::JSON::Object		JSONObject;
    typedef OD::JSON::Array		JSONArray;
    mUseType( TypeSet<int>,		size_type );

    virtual		~ServerProgTool();

    void		exitWithUsage() const;
    void		exitProgram(bool success) const;

#   define		mDeclServerProgToolSetFn( typ ) \
    void		set(const char* keyw,typ,JSONObject* =nullptr)
#   define		mDeclServerProgToolSetArrFn( typ ) \
    void		set(const char* keyw,const typ*,size_type, \
			    JSONObject* =nullptr)

#   define		mDeclServerProgToolSetFns( typ ) \
			mDeclServerProgToolSetFn(typ); \
			mDeclServerProgToolSetFn(const TypeSet<typ>&); \
			mDeclServerProgToolSetArrFn(typ)

			mDeclServerProgToolSetFn(const char*);
			mDeclServerProgToolSetFn(const BufferStringSet&);
			mDeclServerProgToolSetFn(const DBKey&);
			mDeclServerProgToolSetFn(const DBKeySet&);
			mDeclServerProgToolSetFn(bool);
			mDeclServerProgToolSetFn(const BoolTypeSet&);
    void		set(const char*,const bool*,size_type,
			    JSONObject* =nullptr);
			mDeclServerProgToolSetFns(od_int16);
			mDeclServerProgToolSetFns(od_uint16);
			mDeclServerProgToolSetFns(od_int32);
			mDeclServerProgToolSetFns(od_uint32);
			mDeclServerProgToolSetFns(od_int64);
			mDeclServerProgToolSetFns(float);
			mDeclServerProgToolSetFns(double);

			// only in jsonmode_:
    void		set(const char* keyw,JSONObject*);
    void		set(const char* keyw,JSONArray*);

    void		respondInfo(bool success,bool exit=true) const;
    void		respondError(const char*) const;
    void		respondError(const uiRetVal&) const;

    CommandLineParser&	clp()		    { return *clp_; }
    const CommandLineParser& clp() const    { return *clp_; }
    od_istream&		inStream() const;
    od_ostream&		outStream() const;

    static const char*	sListSurvCmd()	    { return "list-surveys"; }
    static const char*	sListUsrCmd()	    { return "list"; }
    static const char*	sExistsUsrCmd()     { return "exists"; }
    static const char*	sInfoUsrCmd()	    { return "info"; }
    static const char*	sAllUsrCmd()	    { return "all"; }
    static const char*	sKeyTransGrp(int n=1)
			{ return n<2 ? "TranslatorGroup"
				     : "TranslatorGroups"; }

    void		setDBMDataSource();

protected:

			ServerProgTool(int,char**,const char* moddep);
    void		initParsing(int protocolnr,bool setdatasrc=true);

    CommandLineParser*	clp_;
    bool		jsonmode_ = true;
    JSONObject&		jsonroot_;
    IOPar		iop_;
    int			protocolnr_		    = 0;
    Timer&		timer_;
    int			retval_ =		    -1;

    virtual BufferString getSpecificUsage() const   = 0;

    BufferString	getKeyedArgStr(const char* ky,
				       bool mandatory=true) const;
    DBKey		getDBKey(const char* ky,bool mandatory=true) const;

    static void		addToUsageStr(BufferString&,const char* flg,
				   const char* args,bool isextra=false);
    void		setStatus(bool) const;
    void		timerTickCB(CallBacker*);
    void		exitProgram(bool success);

    template <class T>
    void		setSingle(const char*,T,JSONObject*);
    template <class T>
    void		setArr(const char*,const T&,JSONObject*);
    template <class T>
    void		setArr(const char*,const T*,size_type,JSONObject*);

};
