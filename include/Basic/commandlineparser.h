#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "convert.h"
#include "dbkey.h"
#include "genc.h"
#include "bufstringset.h"
#include "typeset.h"
#include "debug.h" // easier for test programs, declares od_init_test_program

/*!Parser that takes the argc and argv and makes them parsable. An argument
   starting with -- is considered a key, as well as arguments starting with "-",
   if not imedialtely followed by a number. Hence -create is a key, -9 or -.3
   are not.

   "bin/my_prog --nriter 4 parfile1.par --fast parfile2.par"
   can be parsed as follows:

\code
    CommandLineParser parser;
    parser.setKeyHasValue("nriter"); //Makes it handle "4" as a value

    const BufferString exec = parser.getExecutable(); //returns "bin/my_prog"

    int nriter;
    if ( !parser.getVal( "nriter", nriter ) )
        return false;

    const bool fast = parser.hasKey("fast");

    BufferStringSet parfiles;
    getNormalArguments( parfiles );
    if ( parfiles.isEmpty() ) //Will have "parfile1.par" and "parfile2.par"
        return false;

\endcode
 */


mExpClass(Basic) CommandLineParser
{
public:
				CommandLineParser(const char*);
				CommandLineParser(int argc,char** argv);
				CommandLineParser();
				/*!<Actual command line is used, i.e. the one
				    set by SetProgramArgs */
    virtual			~CommandLineParser();
				mOD_DisableCopy(CommandLineParser)

    void			setKeyHasValue(const char* key,
					       int nrvals=1) const;
				/*!<Tell the parser that the nrvals arguments
				    after key are values. nrvals<1 denotes a
				    variable number of values, running up to
				    the next key. This function is only needed
				    if you will use getNormalArguments. */
    void			getNormalArguments(BufferStringSet&) const;
				/*!<Gets all arguments that are not keys or
				    key-values. */

    bool			hasKey(const char*) const;
    // Extract "valnr"ed string after last occurrence of "key" in commandline
    bool			getVal(const char* key,BufferString&,
				       bool acceptnone=false,int valnr=1) const;
    // Extract all string arguments after all occurrences of "key"
    bool			getVal(const char* key,BufferStringSet&,
				       bool acceptnone=false) const;
    bool			getVal(const char* key,DBKey&,
				       bool acceptnone=false,int valnr=1) const;
    template <class T> bool	getVal(const char* key,T&,
				       bool acceptnone=false,int valnr=1) const;
				/*!<Will parse argument valnr following key.
				    If acceptnone is true, it will only give
				    error if key is found, but no value can be
				    parsed. */
    // Extract all type <T> arguments after all occurrences of "key"
    template <class T> bool	getVal(const char* key,TypeSet<T>&,
				       bool acceptnone=false) const;

				// following return empty string, invalid DBKey
				// or Udf if the key is not present or
				// carries no value
    BufferString		keyedString(const char* ky,int argnr=0) const;
    template <class T>
    T				keyedValue(const char* ky,int argnr=0) const;

    bool			isPresent(const char*) const;
				//!<Is string present as an argument.

    int				nrArgs() const		{ return argv_.size(); }
				/*!<\returns the lump sum (keys, values, and
				    everything else, but program name */

    bool			isKey(int) const;
				//!<Does the arg start with - or  --
    bool			isKeyValue(int idx) const;
				/*!<True if not a key, and previous is a key
				    that has been set using setKeyHasValue. */

    const OD::String&		getArg(int idx) const	{ return *argv_[idx]; }
    const OD::String&		lastArg() const;

    const OD::String&		getExecutable() const;
    const OD::String&		getExecutableName() const;

    mDeprecated			("Use BufferString createKey(const char*)")
    static void			createKey(const char* key,BufferString& res)
				{ res = "--"; res += key; }
    static BufferString		createKey( const char* key )
				{ return BufferString("--",key); }

    static void			addKey(const char* key,BufferString& cmd,
				       const char* valstr=0);
				//!<adds a space before but not after
    static void			addFilePath(const char*,BufferString& cmd);
				/*!<adds "\ and \" to protect for spaces
				    in FilePaths */


    static const char*		sDataRootArg()	   { return "dataroot"; }
    static const char*		sSurveyArg()	   { return "survey"; }
    static const char*		sNeedTempSurv()    { return "needtempsurvey"; }
    BufferString		envVarBase() const;
    mDeprecatedObs
    BufferString		getFullSurveyPath(bool* iscursur=nullptr) const;

private:

    int				indexOf(const char*,
					TypeSet<int>* idxs=nullptr) const;
    void			init(int,char**);
    void			init(const char*);
    void			ensureNrArgs(const char*,int) const;

    BufferString		progname_;
    BufferString		executable_;
    BufferStringSet		argv_;

    BufferStringSet		keyswithvalue_;
    TypeSet<int>		nrvalues_;
};

//Implementation

template <class T> inline
bool CommandLineParser::getVal( const char* key, T& val,
				bool acceptnone, int valnr ) const
{
    const int keyidx = indexOf( key );
    if ( keyidx<0 )
	return acceptnone;

    const int validx = keyidx + mMAX(valnr,1);
    if ( !argv_.validIdx( validx ) || isKey(validx) )
	return false;

    val = Conv::to<T>( argv_[validx]->buf() );

    return !mIsUdf(val);
}


template <class T> inline
bool CommandLineParser::getVal( const char* key, TypeSet<T>& vals,
				bool acceptnone ) const
{
    vals.setEmpty();
    TypeSet<int> keyidxs;
    const int keyidx = indexOf( key, &keyidxs );
    if ( keyidx<0 )
	return acceptnone;

    for ( int kidx=0; kidx<keyidxs.size(); kidx++ )
    {
	int validx = keyidxs[kidx] + 1;
	while ( argv_.validIdx( validx ) && !isKey(validx) )
	{
	    vals += Conv::to<T>( argv_[validx]->buf() );
	    validx++;
	}
    }

    return acceptnone ? true : !vals.isEmpty();
}


inline BufferString CommandLineParser::keyedString( const char* ky,
						    int argnr ) const
{
    ensureNrArgs( ky, argnr+1 );
    BufferString ret;
    getVal( ky, ret, false, argnr );
    return ret;
}


template <class T> inline
T CommandLineParser::keyedValue( const char* ky, int argnr ) const
{
    ensureNrArgs( ky, argnr+1 );
    T ret = mUdf(T);
    getVal( ky, ret, false, argnr );
    return ret;
}
