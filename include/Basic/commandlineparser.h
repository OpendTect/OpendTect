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

#include "bufstringset.h"

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
    if ( !parser.get( "nriter", nriter ) )
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
    
    void			setKeyHasValue(const char* key);
				/*!<Tell the parser that the argument after key
				    is a value. This is only neeed if you
				    will use getNormalArguments. */
    void			getNormalArguments(BufferStringSet&) const;
				/*!<Gets all arguments that are not keys or
				    key-values. */
    
    bool			hasKey(const char*) const;
    template <class T> bool	getVal(const char* key,T&) const;
				//!<Will parse the argument following key
    
    bool			isPresent(const char*) const;
				//!<Is string present as an argument.
    
    int				nrArgs() const		{ return argv_.size(); }
				/*!<\returns the lump sum (keys, values, and
				    everything else, but program nam	e */
    
    bool			isKey(int) const;
				//!<Does the arg start with - or  --
    bool			isKeyValue(int idx) const;
				/*!<True if not a key, and previous is a key
				    that has been set using setKeyHasValue. */
    
    const BufferString&		getArg(int idx) const	{ return *argv_[idx]; }
    const BufferString&		lastArg() const;
    
    const BufferString&		getExecutable() const;
    const BufferString&		getExecutableName() const;

private:
    
    int				indexOf(const char*) const;
    void			init(int,char**);

    BufferString		progname_;
    BufferString		executable_;
    BufferStringSet		argv_;
    
    BufferStringSet		keyswithvalue_;
};

//Implementation

template <class T> inline
bool CommandLineParser::getVal( const char* key, T& val ) const
{
    const int keyidx = indexOf( key );
    if ( keyidx<0 )
	return false;

    const int validx = keyidx+1;
    if ( !argv_.validIdx( validx ) )
	return false;
    
    if ( isKey(validx) )
	return false;

    return getFromString( val, argv_[validx]->buf() );
}

#endif

