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
#include "basicmod.h"

#include "objectset.h"

class uiStringData;

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

 \note As multiple uiStrings may use the same underlying data (if they are
       constructed with the copy constructor or the equal operator, they are not
       suited for other types of string operations than passing messages to the
       user.
 */


mExpClass(Basic) uiString
{
public:
				uiString(const uiString&);
				/*!<\note Does not copy data, will use the same
				  underlying data structure (reference
				  counted). */
				uiString(const char* original = 0);
				uiString(const FixedString&);
				uiString(const BufferString&);

				~uiString();

    bool			isEmpty() const;

    uiString&			operator=(const uiString&);
				/*!<\note Does not copy data, will use the same
				 underlying data structure (reference
				 counted). */
    uiString&			setFrom(const uiString& src);
				/*!<Copies all data over from src. If src and
				    this use the same data, they will be
				    separated. */
    uiString&			operator=(const char*);
    uiString&			operator=(const FixedString&);

    uiString&			operator=(const BufferString&);
    bool			operator==(const uiString& b) const
				{ return b.data_==data_; }

    uiString			arg(const char*) const;
    uiString			arg(const uiString&) const;

    const char* 		getFullString() const;
				/*!<Constructs the result from the original
				    string and the arguments,
				without translation.
				\note Returned string will only be valid until
				 next call. */
    const char*			getOriginalString() const;
    const mQtclass(QString&)	getQtString() const;

private:
    friend class		uiStringData;
    uiStringData*		data_;
public:
				uiString(const char* original,
					 const char* context,
					 const char* disambiguation,
					 int pluralnr);
				//Only for expert users

};

#define mTextTranslationClass(clss) \
private: \
 static inline uiString tr( const char* text, const char* disambiguation = 0,  \
 int pluralnr=-1 ) \
 { return uiString( text, #clss, disambiguation, pluralnr ); }


#endif

