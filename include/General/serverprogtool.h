#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2019
________________________________________________________________________

-*/

#include "commandlineparser.h"
#include "genc.h"
#include "iopar.h"
#include "odruncontext.h"
class CommandLineParser;
namespace OD { namespace JSON { class Array; class Object; } }

mExpClass(Basic) ServerProgTool
{
public:

    typedef OD::JSON::Object		JSONObject;
    typedef OD::JSON::Array		JSONArray;

    virtual		~ServerProgTool();

    CommandLineParser&	clp()		    { return *clp_; }

    void		exitWithUsage();
    void		exitProgram(bool success);

    void		set(const char* keyw,const char*,JSONObject* jobj=0);
    void		set(const char* keyw,const DBKey&,JSONObject* jobj=0);
    void		set(const char* keyw,int,JSONObject* jobj=0);
    void		set(const char* keyw,float,JSONObject* jobj=0);
    void		set(const char* keyw,double,JSONObject* jobj=0);
    void		set(const char* keyw,const BufferStringSet&,
			    JSONObject* jobj=0);
    void		set(const char* keyw,const DBKeySet&,
			    JSONObject* jobj=0);

			// only in jsonmode_:
    void		set(const char* keyw,JSONObject*);
    void		set(const char* keyw,JSONArray*);

    void		respondInfo(bool success,bool exit=true);
    void		respondError(const char*);

protected:

			ServerProgTool(int,char**,const char* moddep,
					bool jsonmodebydefault=true);
    void		initParsing(int protocolnr);

    const bool		jsonisdefault_;
    CommandLineParser*	clp_;
    bool		jsonmode_;
    JSONObject&		jsonroot_;
    IOPar		iop_;
    int			protocolnr_		    = 0;

    virtual BufferString getSpecificUsage() const   = 0;
    static void		addToUsageStr(BufferString&,const char* flg,
				   const char* args,bool isextra=false);
    void		setStatus(bool);

    template <class T>
    void		setSingle(const char*,T,JSONObject*);

};
