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
#include "keystrs.h"

class uiStringData;

mFDQtclass( QString );
mFDQtclass( QTranslator );


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

 The translation in OpendTect is done using Qt's subsystem for localization.
 A class that wishes to enable localization should:

  -# Declare the mTextTranslationClass(classname,application) in its class
     definition. The application is a string that identifies your application.
     OpendTect's internal classes use the "od" applicaiton string, and can for
     short use the mODTextTranslationClass macro.
  -# Use the tr() function for all translatable string. The tr() function
     returns a uiString() that can be passed to the ui.
  -# Use Qt's lupdate to scan your code for localization strings. This will
     generate a .ts file which can be editded with Qt's Linguist program to
     translate the strings.
  -# The updated .ts file should be converted to a binary .qm file using Qt's
     lrelease application.
  -# The .qm file should be placed in
     data/localizations/<application>_<lang>_<country>.ts in the release. For
     example, a localization of OpendTect to traditional Chinese/Taiwan would be
    saved as od_zh_TW.qm.
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
    void			setEmpty() { *this = sKey::EmptyString(); }

    uiString&			operator=(const uiString&);
				/*!<\note Does not copy data, will use the same
				 underlying data structure (reference
				 counted). */
    uiString&			setFrom(const uiString& src);
				/*!<Copies all data over from src. If src and
				    this use the same data, they will be
				    separated. */
    void			setFrom(const mQtclass(QString)&);
    uiString&			operator=(const char*);
    uiString&			operator=(const FixedString&);

    uiString&			operator=(const BufferString&);
    bool			operator==(const uiString& b) const
				{ return b.data_==data_; }
    bool			operator!=(const uiString& b) const
				{ return b.data_!=data_; }

    uiString&			arg(const char*);
    uiString&			arg(const FixedString&);
    uiString&			arg(const BufferString&);
    uiString&			arg(const uiString&);

    uiString&			append(const char*);
				/*!Appends. In most cases, use arg to allow
				  translator to change ordering. */
    uiString&			append(const FixedString&);
				/*!Appends. In most cases, use arg to allow
				translator to change ordering. */
    uiString&			append(const BufferString&);
				/*!Appends. In most cases, use arg to allow
				 translator to change ordering. */
    uiString&			append(const uiString&);
				/*!Appends. In most cases, use arg to allow
				 translator to change ordering. */

    const BufferString& 	getFullString() const;
				/*!<Constructs the result from the original
				    string and the arguments,
				without translation.
				\note Returned string will only be valid until
				 next call. */
    const char*			getOriginalString() const;
    const mQtclass(QString&)	getQtString() const;
    wchar_t*			createWCharString() const;
				/*!<Result becomes owners and should be
				    deleted using the [] operator. */

    static const char*		sODLocalizationApplication() { return "od"; }

private:
    friend class		uiStringData;
    uiStringData*		data_;
public:
				//Only for expert users
				uiString(const char* original,
					 const char* context,
					 const char* application,
					 const char* disambiguation,
					 int pluralnr);
    void			addLegacyVersion(const uiString&);
				/*!If this string was previously known by
                                   another origianl string, it can be added here
                                */

    bool			translate(const mQtclass(QTranslator)&,
					  mQtclass(QString)&) const;
				//!Returns true if the translation succeeded



};

/*!Wrapper class with fewer constructor that ensures that it is completely
   decoupled from its input. */

mExpClass(Basic) uiStringCopy : public uiString
{
public:
				uiStringCopy(const uiString& in)
				    : uiString( (const char*) 0 )
				{ setFrom(in); }
				uiStringCopy(const char* in = 0)
				    : uiString(in)
				{}
				uiStringCopy(const FixedString& in)
				    : uiString(in)
				{}
				uiStringCopy(const BufferString& in)
				    : uiString(in)
				{}


};

#define mTextTranslationClass(clss,application) \
private: \
 static inline uiString tr( const char* text, const char* disambiguation = 0,  \
 int pluralnr=-1 ) \
 { return uiString( text, #clss, application, disambiguation, pluralnr ); }

#define mODTextTranslationClass(clss) \
mTextTranslationClass( clss, uiString::sODLocalizationApplication() )



#endif

