#ifndef argvparser_h
#define argvparser_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		March 2013
 RCS:		$Id$
________________________________________________________________________



-*/

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
				CommandLineParser(int argc,char** argv);
				CommandLineParser();
				/*!<Actual command line is used, i.e. the one
				    set by SetProgramArgs */

    void			setKeyHasValue(const char* key,int nrvals=1);
				/*!<Tell the parser that the nrvals arguments
				    after key are values. nrvals<1 denotes a
				    variable number of values, running up to
				    the next key. This function is only needed
				    if you will use getNormalArguments. */
    void			getNormalArguments(BufferStringSet&) const;
				/*!<Gets all arguments that are not keys or
				    key-values. */

    bool			hasKey(const char*) const;
    bool			getVal(const char* key,BufferString&,
				       bool acceptnone=false,int valnr=1) const;
    template <class T> bool	getVal(const char* key,T&,
				       bool acceptnone=false,int valnr=1) const;
				/*!<Will parse argument valnr following key.
				    If acceptnone is true, it will only give
				    error if key is found, but no value can be
				    parsed. */

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

    static void			createKey(const char* key,BufferString& res)
				{ res = "--"; res += key; }

private:

    int				indexOf(const char*) const;
    void			init(int,char**);

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

    return getFromString( val, argv_[validx]->buf(), mUdf(T) );
}

bool CommandLineParser::getVal( const char* key, BufferString& val,
				bool acceptnone, int valnr ) const
{
    const int keyidx = indexOf( key );
    if ( keyidx<0 )
	return acceptnone;

    const int validx = keyidx + mMAX(valnr,1);
    if ( !argv_.validIdx( validx ) || isKey(validx) )
	return false;

    val.set( argv_[validx]->buf() );
    return true;
}

#endif

