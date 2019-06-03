#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2019
________________________________________________________________________

-*/

#include "generalmod.h"

#include "commandlineparser.h"
#include "genc.h"
#include "iopar.h"
#include "odruncontext.h"
#include "od_iosfwd.h"
class CommandLineParser;
class uiRetVal;
namespace OD { namespace JSON { class Array; class Object; } }

mExpClass(General) ServerProgTool
{
public:

    typedef OD::JSON::Object		JSONObject;
    typedef OD::JSON::Array		JSONArray;
    typedef int				size_type;

    virtual		~ServerProgTool();

    void		exitWithUsage();
    void		exitProgram(bool success);

#   define		mDeclServerProgToolSetFn( typ ) \
    void		set(const char* keyw,typ,JSONObject* jobj=0)
#   define		mDeclServerProgToolSetArrFn( typ ) \
    void		set(const char* keyw,const typ*,size_type, \
			    JSONObject* jobj=0)

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
    void		set(const char*,const bool*,size_type,JSONObject* o=0);
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

    void		respondInfo(bool success,bool exit=true);
    void		respondError(const char*);
    void		respondError(const uiRetVal&);

    CommandLineParser&	clp()		    { return *clp_; }
    const CommandLineParser& clp() const    { return *clp_; }
    od_istream&		inStream();
    od_ostream&		outStream();

protected:

			ServerProgTool(int,char**,const char* moddep);
    void		initParsing(int protocolnr);

    CommandLineParser*	clp_;
    bool		jsonmode_;
    JSONObject&		jsonroot_;
    IOPar		iop_;
    int			protocolnr_		    = 0;

    virtual BufferString getSpecificUsage() const   = 0;

    BufferString	getKeyedArgStr(const char* ky,
				       bool mandatory=true) const;
    static void		addToUsageStr(BufferString&,const char* flg,
				   const char* args,bool isextra=false);
    void		setStatus(bool);
    void		setSize(size_type);

    template <class T>
    void		setSingle(const char*,T,JSONObject*);
    template <class T>
    void		setArr(const char*,const T&,JSONObject*);
    template <class T>
    void		setArr(const char*,const T*,size_type,JSONObject*);

};
