#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2014
________________________________________________________________________

-*/

#include "gendefs.h"
#include "keystrs.h"
#include "threadlock.h"
#include "string2.h"
#include "typeset.h"

class uiStringData;

mFDQtclass( QString );
mFDQtclass( QStringList );
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
   uiString string = toUiString( "%1 plus %2 is %3")
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
  -# For functions not belonging to a class, use the od_static_tr function.
     Any function matching the *_static_tr will be interpreted by lupdate.
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
		~uiString();

    uiString&	set(const char*);
    inline bool	isSet() const			{ return !isEmpty(); }
    bool	isEmpty() const;
    void	setEmpty();
    uiString&	toLower(bool yn=true);
		/*!<Set output to lowercase, if allowed by language
		    This is applied before eventual arguments. */
    bool	operator!() const		{ return isEmpty(); }

    uiString&	operator=(const uiString&);	//!< no copy, ref counted
    bool	operator>(const uiString& b) const;
    bool	operator<(const uiString& b) const;
    int		size() const;
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
    const OD::String&		getFullString(BufferString* = 0) const;
				/*!< Full string, *without* translation
				    result is in a thread-safe static buffer,
				    so copy the result before calling again.
				    If BufferString is given, it will be
				    filled, and static buffer won't be used. */
    wchar_t*			createWCharString() const;
				/*!< The translation. Result becomes owner's and
				    should be deleted using the [] operator. */
    const char*			getOriginalString() const;
    const mQtclass(QString)&	getQString() const;
	/*!<Returns reference, so could be unsafe */
    const mQtclass(QString)&	fillQString(QString&) const;
				/*!<Fully thread-safe. Returns input*/
    const BufferString&		fillUTF8String(BufferString&) const;
				/*!< Full string, with translation
				    result. Returns input. */

    void			getHexEncoded(BufferString&) const;
				/*!<Encodes translated string into a const char*
				    buffer that can has only 0-9 A-F */
    bool			setFromHexEncoded(const char*);
				//!Reads hex-data and sets the translated str.
private:

    inline			operator bool() const	{ return !isEmpty(); }
    bool			isCacheValid() const;
    const mQtclass(QString)&	getQStringInternal() const;

    friend class		uiStringData;

    char*			debugstr_;
				/*<!< Contains getFullString() for easy debuggin
				      Only filled in in debug builds. */

    mutable uiStringData*	data_;
    mutable Threads::Lock	datalock_;	//!< Protects data_ variable
    static const uiString	emptystring_;

public:

		//Only for expert users
    mDeprecated			uiString(const char* inp);
    mDeprecated			uiString(const OD::String&);
    mDeprecated uiString&	operator=(const char*);
    mDeprecated uiString&	operator=(const OD::String&);

		/*!<Don't use. May be depreciated. Use toUiString("My text")
		    function instead. */

    bool	operator==(const uiString& b) const;
		//!<Don't use, will force crash. Only here to keep TypeSet happy
    bool	operator!=(const uiString& b) const { return !(*this==b); }
		//!<Don't use, will force crash. Only here to keep TypeSet happy
    bool	isEqualTo(const uiString& oth) const;
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
		/*!<Set the translated text. No further
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

    uiString&	addSpace(int nr=1);
};



/*\brief Set of uiStrings */

mExpClass(Basic) uiStringSet : public TypeSet<uiString>
{ mODTextTranslationClass(uiStringSet);
public:
		uiStringSet()				{}
		uiStringSet( const uiString* );
		/*!<Adds list of strings until an empty string is found. */
		uiStringSet( const uiStringSet& sl )
		    : TypeSet<uiString>(sl)		{}

    uiString	createOptionString(bool use_and=true,int maxnritems=-1,
				   char space = ' ') const;
		//!<Returns a string with "option1, option2, and/or option 3"
    void	fill(mQtclass(QStringList)&) const;
    uiString	cat(const char* sepstr="\n") const;
};


/*\brief allows returning status and accompanying user info.

  This class helps us make sure there is always user info on errors. Therefore,
  you will find a 'setIsOK' but not OK means setting a non-empty message.

*/

mExpClass(Basic) uiRetVal
{
public:

			uiRetVal(const uiString&);
			uiRetVal(const uiStringSet&);
			uiRetVal(const uiRetVal&);
    static uiRetVal	OK()		    { return ok_; }
    uiRetVal&		operator =(const uiRetVal&);
    uiRetVal&		operator =(const uiString&);
    uiRetVal&		operator =(const uiStringSet&);
			operator uiString() const;

    bool		isOK() const;
    inline bool		isError() const	    { return !isOK(); }
    bool		isMultiMessage() const;
    uiStringSet		messages() const;

    uiRetVal&		setOK();
    uiRetVal&		insert(const uiString&);
    uiRetVal&		set(const uiRetVal&);
    uiRetVal&		set(const uiString&);
    uiRetVal&		set(const uiStringSet&);
    uiRetVal&		add(const uiRetVal&);
    uiRetVal&		add(const uiString&);
    uiRetVal&		add(const uiStringSet&);
    void		resetError() { msgs_.setEmpty(); }

    BufferString	getText() const;

private:

    uiStringSet		msgs_;
    mutable Threads::Lock lock_;

			uiRetVal()	    {}
    static const uiRetVal ok_;

};


mGlobal(Basic) uiString toUiString(const uiString&);
mGlobal(Basic) uiString toUiString(const char*);
mGlobal(Basic) uiString toUiString(const OD::String&);
mGlobal(Basic) uiString toUiString(od_int32);
mGlobal(Basic) uiString toUiString(od_int64);
mGlobal(Basic) uiString toUiString(od_uint32);
mGlobal(Basic) uiString toUiString(od_uint64);
mGlobal(Basic) uiString toUiString(float);
mGlobal(Basic) uiString toUiString(double);
mGlobal(Basic) uiString toUiString(float,int nrdec);
mGlobal(Basic) uiString toUiString(double,int nrdec);


//User when string should be revisited later
#define mToUiStringTodo(i) ::toUiString(i)
#define mFromUiStringTodo(i) i.getFullString()


//Legacy Will be removed
mGlobal(Basic) mDeprecated inline uiString mkUiString(const char* var)
{ return toUiString(var); }

/*!Adds translation of strings outside of classes for the "od" application. It
   will return a uistring where the context is "static_func_function", where
   'function' is whatever is given as the function parameter. This matches what
   is done in the filtering of the source files before lupdate is run (in
   ./dtect/update_translations.csh).

   \code
   bool myFunction( int var )
   {
       if ( var<5 )
       {
	   uiMSG().error(
	       od_static_tr( "myFunction", "%1 is less than 5" ).arg( var ) );
       }
   }

   \endcode
*/

mGlobal(Basic) uiString od_static_tr( const char* function, const char* text,
	const char* disambiguation = 0, int pluralnr=-1 );
mGlobal(Basic) uiString getUiYesNoString(bool res);

template <class T> inline
uiString& uiString::arg( const T& var )
{
    return arg( toUiString(var) );
}
