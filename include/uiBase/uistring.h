#ifndef uistring_h
#define uistring_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "gendefs.h"
#include "uibasemod.h"

mFDQtclass( QString )

/*!String that is able to hold wide character strings for the user interface.
   These strings can be in different encodings and should only be used to pass
   text to the ui.

   The string may have an original string encoded in ASCI characters, but that
   should only be used for object names and similar.

   If the string holds %N arguments, these can be replaced by arguments:

 \code
   uiString string = uiString( "%1 plus %2 is %3")
			.arg( toString(4) )
			.arg( toString(5) )
			.arg( toString(4+5) );
 \endcode

   Will result in the string "4 plus 5 is 9"
 */


mExpClass(uiBase) uiString
{
public:
			uiString(const uiString&);
			uiString(const char* = 0);
			/*!<String is assumed to stay alive as it will be used
			    for the originalstring_. */
			uiString(const FixedString&);
			/*!<Underlying const char is assumed to stay alive and
			    will be used for the originalstring_ */
			uiString(const BufferString&);
			uiString(const QString&,
				 const char* originalstring=0);
			~uiString();

    uiString&		operator=(const uiString&);
    uiString&		operator=(const char*);
			/*!<String is assumed to stay alive as it will be used
			 for the originalstring_. */
    uiString&		operator=(const FixedString&);
			/*!<Underlying const char is assumed to stay alive and
			    will be used for the originalstring_ */
    uiString&		operator=(const BufferString&);


    uiString		arg(const char*) const;
    uiString		arg(const uiString&) const;

    mQtclass(const QString&)	getQtString() const	  { return *qstring_; }
    const char*			getOriginalString() const;

private:
    mQtclass(QString*)		qstring_;
    const char*			originalstring_;
};

#endif

