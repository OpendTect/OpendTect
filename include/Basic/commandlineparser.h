#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		March 2013
________________________________________________________________________



-*/

#include "convert.h"
#include "bufstringset.h"
#include "typeset.h"
#include "dbkey.h"
#include "debug.h" // easier for test programs, declares od_init_test_program

/*! takes the argc and argv and makes them queryable. An argument
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

  Note: if there is an environment variable executablename_ARGS then that will
  overrule any provided args. Further, executablename_EXTRA_ARGS will keep
  the args but adds the specified string to the command line.

  Note 2: From od 7 you can no longer get the value following a key without
  making the parser aware of it. You can either:
  * use keyed[String|Value|DBKey] (these will return empty, invalid or Udf if
	the flag is not there or unparseable)
  * announce up front using setKeyHasValue(s) and then use getKeyedInfo
	afterwards.

\code

    const BufferString sval = clp.keyedString( "sinp" );
    if ( sval.isEmpty() )
	err( "Please provide valid 'sinp' argument" );

    const int ival = clp.keyedValue<int>( "iinp" );
    if ( mIsUdf(ival) )
	err( "Please provide valid 'iinp' argument" );

    clp.setKeyHasValue( "iinp" );
    if ( clp.hasKey("iinp") )
    {
	int ival;
	if ( !clp.getKeyedInfo("iinp",ival) )
	    err( "Invalid 'iinp' argument" );
    }

\endcode

 */


mExpClass(Basic) CommandLineParser
{
public:

			CommandLineParser();
				/*!< uses the args set by SetProgramArgs */
    bool		hasKey(const char* ky) const;

    void		getNormalArguments(BufferStringSet&) const;
				/*!< all arguments not keys or key-values. */

    void		setKeyHasValue( const char* key ) const
				{ ensureNrArgs( key, 1 ); }
    void		setKeyHasValues( const char* key, int nrvals ) const
				{ ensureNrArgs( key, nrvals ); }

    bool		getKeyedInfo(const char* key,BufferString&,
				       bool acceptnone=false,int argnr=0) const;
    bool		getKeyedInfo(const char* key,DBKey&,
				       bool acceptnone=false,int argnr=0) const;
    template <class T>
    bool		getKeyedInfo(const char* key,T&,bool acceptnone=false,
				       int argnr=0) const;
				/*!<Will parse argument argnr following key.
				    If acceptnone is true, it will only give
				    error if key is found, but no value can be
				    parsed. */

			// following return empty string, invalid DBKey or Udf
			// if the key is not present or carries no value
    BufferString	keyedString(const char* ky,int argnr=0) const;
    DBKey		keyedDBKey(const char* ky,int argnr=0) const;
    template <class T>
    T			keyedValue(const char* ky,int argnr=0) const;

			// Variable length: collect all values up to next key
    BufferStringSet	keyedList(const char* ky) const;

    bool		isPresent(const char*) const;
				//!<Is string present as an argument.

    int			nrArgs() const		{ return argv_.size(); }
				/*!<\returns the lump sum (keys, values, and
				    everything else, but program name */

    bool		isKey(int) const;
				//!<Does the arg start with - or  --
    bool		isKeyValue(int idx) const;
				/*!<True if not a key, and previous is a key
				    that has been set using setKeyHasValue. */

    const OD::String&	getArg(int idx) const	{ return *argv_[idx]; }
    const OD::String&	lastArg() const;

    const OD::String&	getExecutable() const;
    const OD::String&	getExecutableName() const;

    static BufferString	createKey( const char* key )
				{ return BufferString("--",key); }

    int			getArgc() const	    { return argv_.size()+1; }
    char**		getArgv() const;    //!< allocates everything with new
    int			indexOf(const char*) const;

    static const char*	sFileForArgs()	    { return "argsfile"; }
    static const char*	sDataRootArg()	    { return "dataroot"; }
    static const char*	sSurveyArg()	    { return "survey"; }
    BufferString	envVarBase() const;
    BufferString	getFullSurveyPath(bool* iscursurv=0) const;

private:

    void		init(int,char**);
    void		init(const char*);

    BufferString	progname_;
    BufferString	executable_;
    BufferStringSet	argv_;

    BufferStringSet	keyswithvalue_;
    TypeSet<int>	nrvalues_;

    const char*		gtVal(const char*,int,bool&) const;
    void		ensureNrArgs(const char*,int) const;

public:

    explicit		CommandLineParser(const char* fullcommand);
			CommandLineParser(int argc,char** argv);
    void		overruleArgs(const BufferStringSet&,int start_at_arg=0);
    void		overruleArgsIfEnvVarSet(const char* envvarnm);

};


inline bool CommandLineParser::getKeyedInfo( const char* ky, BufferString& val,
					     bool acceptnone, int argnr ) const
{
    bool found;
    const char* str = gtVal( ky, argnr, found );
    if ( found )
	{ val.set( str ); return acceptnone ? !val.isEmpty() : true; }
    return acceptnone;
}


inline bool CommandLineParser::getKeyedInfo( const char* ky, DBKey& val,
					     bool acceptnone, int argnr ) const
{
    bool found;
    const char* str = gtVal( ky, argnr, found );
    if ( found )
	{ val = DBKey( str ); return acceptnone ? val.isValid() : true; }
    return acceptnone;
}

template <class T> inline
bool CommandLineParser::getKeyedInfo( const char* ky, T& val,
				      bool acceptnone, int argnr ) const
{
    bool found;
    const char* str = gtVal( ky, argnr, found );
    if ( found )
	{ val = Conv::to<T>( str ); return !mIsUdf(val); }
    return acceptnone;
}


inline BufferString CommandLineParser::keyedString( const char* ky,
						       int argnr ) const
{
    ensureNrArgs( ky, argnr+1 ); BufferString ret;
    getKeyedInfo( ky, ret, false, argnr ); return ret;
}

inline DBKey CommandLineParser::keyedDBKey( const char* ky, int argnr ) const
{
    ensureNrArgs( ky, argnr+1 ); BufferString str;
    getKeyedInfo( ky, str, false, argnr );
    return DBKey( str );
}

template <class T> inline
T CommandLineParser::keyedValue( const char* ky, int argnr ) const
{
    ensureNrArgs( ky, argnr+1 ); T ret = mUdf(T);
    getKeyedInfo( ky, ret, false, argnr ); return ret;
}
