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
#include "keystrs.h"
#include "threadlock.h"
#include "string2.h"
#include "typeset.h"

class uiStringData;

mFDQtclass( QString );
mFDQtclass( QTranslator );

#define mTextTranslationClass(clss,application) \
private: \
 static inline uiString tr( const char* text, const char* disambiguation = 0,  \
 int pluralnr=-1 ) \
 { return uiString( text, #clss, application, disambiguation, pluralnr ); } \
 static inline uiString legacyTr( const char* text, \
				  const char* disambiguation = 0,  \
				  int pluralnr=-1 ) \
 { return uiString( text, #clss, application, disambiguation, pluralnr ); }

#define mODTextTranslationClass(clss) \
mTextTranslationClass( clss, uiString::sODLocalizationApplication() )


/*!
   String that is able to hold wide character strings for the user interface.
   These strings can be in different encodings and should only be used to pass
   text to the ui.

   The string may have an original string encoded in ASCI characters, but that
   should only be used for object names and similar.

   If the string holds %N arguments, these can be replaced by arguments:

 \code
   uiString string = mkUiString( "%1 plus %2 is %3")
			.arg( 4 )
			.arg( 5 )
			.arg( 4+5 );
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
{ mODTextTranslationClass(uiString);
public:

		uiString();
		uiString(const uiString&);	//!< no copy, ref counted
		uiString(const OD::String&);
		~uiString();

    uiString&	set(const char*);
    bool	isSet() const			{ return !isEmpty(); }
    bool	isEmpty() const;
    void	setEmpty();
    bool	operator!() const		{ return isEmpty(); }

    uiString&	operator=(const uiString&);	//!< no copy, ref counted
    uiString&	operator=(const char*);
    uiString&	operator=(const OD::String&);
    bool	operator>(const uiString& b) const;
    bool	operator<(const uiString& b) const;

    static const uiString& emptyString()	{ return emptystring_; }


	/*! uiStrings should only be manipulated using the arg() functions.
	    These replace the next %N (e.g. %1) with the provided argument. */
    template <class T>
    uiString&	arg(const T& var);
    uiString&	arg(const uiString&);

	/*! append() functions are used to concatenate entire sentences.
	    Otherwise you'll be in trouble because you will not know in what
	    order args end up after translation.
	    'withnewline' will only add one if the first string is not empty. */
    uiString&	append(const char*, bool withnewline=false);
    uiString&	append(const OD::String&, bool withnewline=false);
    uiString&	append(const uiString&, bool withnewline=false);


    /*! Results: */
    const OD::String&	getFullString() const;
				/*!< Full string, *without* translation
				    result is in a thread-safe static buffer,
				    so copy the result before calling again. */
    wchar_t*		createWCharString() const;
				/*!< The translation. Result becomes owner's and
				    should be deleted using the [] operator. */
    const char*		getOriginalString() const;
    const mQtclass(QString)& getQtString() const;

private:

#ifdef __debug__
    char*	str_;		//!< Contains getFullString() for easy debugging
#endif

    friend class		uiStringData;
    uiStringData*		data_;
    mutable Threads::Lock	datalock_;	//!< Protects data_ variable
    static const uiString	emptystring_;

public:

		//Only for expert users

		uiString(const char* inp);
		/*!<Don't use. May be depreciated. Use mkUiString("My text")
		    function instead. */

    bool	operator==(const uiString& b) const;
		//!<Don't use, will force crash. Only here to keep TypeSet happy
    bool	operator!=(const uiString& b) const { return !(*this==b); }
		//!<Don't use, will force crash. Only here to keep TypeSet happy
    inline bool	isEqualTo( const uiString& oth ) const
		{ return getFullString() == oth.getFullString(); }
		//!<Do use, but only if unavoidable

    static const char*	sODLocalizationApplication() { return "od"; }

    void	makeIndependent();
		//!<If data is shared, I'll get an own copy
		uiString(const char* original,
			 const char* context,
			 const char* application,
			 const char* disambiguation,
			 int pluralnr);
    void	setFrom(const mQtclass(QString)&);
		/*!<Set the translated text no further
		    translation will be done. */
    void	addLegacyVersion(const uiString&);
		/*!<If this string was previously known by another origial
		    string, it can be added here. This is normally done with the
		    legacyTr function.
		    \code
			uiString str = tr("New version");
			str.addLegacyVersion( legacyTr("Old ver") );
		    \endcode
                */

    bool	translate(const mQtclass(QTranslator)&,
			  mQtclass(QString)&) const;
		//!<Returns true if the translation succeeded

    static uiString getOrderString(int);
		//Returns 1st, 2nd, 3rd
};


mExpClass(Basic) uiStringSet : public TypeSet<uiString>
{ mODTextTranslationClass(uiStringSet);
public:
		uiStringSet()				{}
		uiStringSet( const uiStringSet& sl )
		    : TypeSet<uiString>(sl)		{}
		uiStringSet( const uiString& s )	{ add( s ); }

    uiString	createOptionString(bool use_and=true,int maxnritems=-1,
				   char space = ' ') const;
		//!<Returns a string with "option1, option2, and/or option 3"

};


mGlobal(Basic) uiString mkUiString(const char* var);


template <class T> inline
uiString& uiString::arg( const T& var )
{
    return arg( mkUiString(toString(var)) );
}


#endif
