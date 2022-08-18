#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "string2.h"
#include "threadlock.h"
#include "bufstring.h"
#include "keystrs.h"

class BufferStringSet;
class uiStringData;
class uiStringSet;
class uiRetVal;
namespace Pos { class IdxPair; }

mFDQtclass( QString );
mFDQtclass( QTranslator );

#define mTextTranslationClass(clss,pkgkey) \
private: \
static inline uiString tr( const char* text, \
			   const char* disambiguation = nullptr, \
			   int pluralnr=-1 ) \
{ return uiString( text, #clss, pkgkey, disambiguation, pluralnr ); } \
static inline uiString legacyTr( const char* text, \
				  const char* disambiguation = nullptr,  \
				  int pluralnr=-1 ) \
{ return uiString( text, #clss, pkgkey, disambiguation, pluralnr ); }

#define mODTextTranslationClass(clss) \
mTextTranslationClass( clss, uiString::sODLocalizationApplication() )


/*!\brief String that is able to hold international (UTF-8) strings for the
   user interface.

   uiString is reference counted; Making a copied object is cheap.

   The string will have an 'original string' of simple ASCI characters, which
   can be used as a 'key' but never for user display.

   The string can have %1, %2, ... arguments. For these, sooner or later
   arguments must be provided, as in:
   uiString string = tr("%1 plus %2 is %3").arg( 4 ).arg( 5 ).arg( 4+5 );
   ... will result in the string "4 plus 5 is 9"

   As such, uiString's are suited for limited text manipulation. But if we
   are doing this for translated words and phrases, beware that alteration and
   combining is not right. Tranalations allow:
   * Entire phrases to be concatenated
   * Construction of lists with certain 'reasonable' separators.
   * In no case, just glue together words and/or verbs into your own phrases.

  The translation in OpendTect is done using Qt's subsystem for localization.
  A class that needs to use localized strings should:

  -# In many cases, you can pick your stuff from the uiStrings common words
     and phrases defined in uistrings.h.
  -# If you need specific words or phrases not in uistrings.h, then use the
	mTextTranslationClass(classname,packagekey)
     macro. The packagekey is a string that identifies your software
     package (probably a short plugin name). OpendTect's internal classes use
     the "od" package string, and can for short use the mODTextTranslationClass
     macro.
  -# Use the tr() function for all non-standard strings. The tr() function
     returns a uiString that can be passed to the ui.
  -# For functions not belonging to a class, use the od_static_tr function.
     Any function matching the *_static_tr will be interpreted by Qt's lupdate.

Notes:

* Use the pre-translated strings and phrases in uistrings.h as often as
  possible. Examples: uiStrings::phrCannotRead() and uiStrings::sTraceNumber().

* Do not juggle with words to construct phrases. This will lead to bad
  translations, sometimes hilarious stuff. You can however join multiple phrases
  into one error message. Prefer this style:

    uiString msg( tr("Rendering %1 data in background").arg(objnm) );
    msg.appendPhrase( tr("When finished, the image will appear") );

  Note the absence of '.' at the end of lines - these phrase terminators should
  be added if necessary depending on the situation. Do not put them in tr()
  messages. There are options for different separators when appending.

* All uiStrings:sXXX() start with a capital. To get the no-caps or all-caps
  versions, simply use toLower() and toUpper(), as in:
    uiStrings::sColumn().toLower()

* Units of measure are not translated. Some common units may have a translation,
  if they are not abbreviated. For example, 'ft' is not translated, 'Feet' is.
  Easy to use is uiStrings::sFeet( bool abbreviation=true );

* You can add 'delimiters' and 'embedding' to words ot phrases. Try to not
  juggle with these yourself, make sure things are handled in a decent,
  standard way. For example, there is a uiString::withUnit() that adds the
  unit (if not empty) with parentheses. Also look at parenthesize(), embed(),
  optional(), ...

* m3Dots: This is only for menus. For buttons, the last argument 'immediate'
  determines the possible 3 dots at the end of the text. m3Dots is therefore
  for menu items only (which actually should have been designed to use the same
  construction, a design error). Set the flag to false and the user will expect
  a new dialog to pop up rather than immediate action. So the button object
  will add the '...' if that is the style (but that may change).

 */


mExpClass(Basic) uiString
{ mODTextTranslationClass(uiString)
public:

		uiString();
		uiString(const uiString&);	//!< no copy, ref counted
		~uiString();
    bool	operator==( const uiString& oth ) const
						{ return isEqualTo(oth); }
    bool	operator!=( const uiString& oth ) const
						{ return !isEqualTo(oth); }
    uiString&	operator=(const uiString&);	//!< no copy, ref counted

#ifdef mNoTranslation
		uiString(const char* str)	{ set( str ); }
    uiString&	operator =(const char* str)	{ set( str ); return *this; }
#else
		uiString(const char*)		= delete; // try 'set()'
    uiString&	operator =(const char*)		= delete; // try 'set()'
#endif

    uiString&	set(const char*);
    uiString&	set( const uiString& s )	{ return (*this = s); }
    bool	isEmpty() const;
    void	setEmpty();
    uiString&	toLower(bool yn=true);		//!< applied before arg subst.
    uiString&	toUpper( bool yn=true )		{ return toLower(!yn); }
    bool	operator!() const		{ return isEmpty(); }

    bool	operator>(const uiString& b) const;
    bool	operator<(const uiString& b) const;
    int		size() const;
    bool	isPlainAscii() const;

    static const uiString&  empty()		{ return emptystring_; }
    static uiString&	    dummy()		{ return dummystring_; }


			/*! the arg() functions allow numbers, other uiStrings,
			    and external strings (like names) to be inserted.
			    See class remarks for example. */

    template <class T>
    inline uiString&	arg(const T&);
    inline uiString&	arg(float,int nrdecimals);
    inline uiString&	arg(double,int nrdecimals);
    uiString&		arg(const uiString&);


#   define		muiStringAppendDefArgs \
			    SeparType septyp=uiString::CloseLine, \
			    AppendType apptyp=uiString::OnNewLine

			/*! appendXX() functions should be used to concatenate
			    entire sentences. You cannot just mix&match words
			    or groups of words at the risk of hilarious
			    translations. */

    enum SeparType	{ NoSep, CloseLine, Space, Tab, Comma, MoreInfo,
			  SemiColon };
    enum AppendType	{ OnSameLine, OnNewLine, AfterEmptyLine };

    uiString&		appendPhrase(const uiString&,muiStringAppendDefArgs);
    uiString&		appendPhrases(const uiStringSet&,
				      muiStringAppendDefArgs);
    uiString&		appendPlainText(const char*,bool addspace=false,
					bool addquotes=false);
    uiString&		appendPlainText(const OD::String&,bool addspace=false,
					bool addquotes=false);

    uiString&		appendPhraseSameLine(const uiString&);
    uiString&		appendAfterList(const uiString&);
    uiString&		constructWordWith(const uiString&,bool addspace=false);
    uiString&		preFixWord(const uiString&);
    uiString&		postFixWord(const uiString&);
    uiString&		embed(const char* open,const char* close);
    uiString&		quote(bool single);
    uiString&		parenthesize();
    uiString&		optional();
    uiString&		embedFinalState();
    uiString&		withUnit(const char*); //!< just the unit itself, please
    uiString&		withUnit(const uiString&);
    uiString&		withSurvZUnit();
    uiString&		withSurvXYUnit();
    uiString&		withSurvDepthUnit();
    uiString&		withNumber(od_int64); // simple add number like "well 1"
    uiString&		addMoreInfo(const uiString&,bool newline=false);
			//!< will deliver ": the string with more info"
    uiString&		addMoreInfo(const char*);
    template <class T>
    uiString&		addMoreInfo(const T&);

    inline uiString&	appendIncorrect(const uiString&,char sep=' ');
    inline uiString&	appendIncorrect(const char*,char sep=' ');
    inline uiString&	appendIncorrect(const OD::String&,char sep=' ');

    // TEMP-- comment out to fix a directory at a time
    inline uiString&	append( const uiString& s, bool nl=false )
				{ return appendIncorrect(s,nl?'\n':' '); }
    inline uiString&	append( const char* s, bool nl=false )
				{ return appendIncorrect(s,nl?'\n':' '); }
    inline uiString&	append( const OD::String& s, bool nl=false )
				{ return appendIncorrect(s,nl?'\n':' '); }
    inline uiString&	addSpace( int =1 )	{ return append(" "); }
    inline uiString&	addTab( int =1 )	{ return append("\t"); }
    inline uiString&	addNewLine( int =1 )	{ return append("\n"); }
    // End TEMP

    static uiString	getOrderString(int);	//!< 1st, 2nd, 3rd, ...

			// UNtranslated:
    const char*		getOriginalString() const;
			//!< *untranslated*, no argument substitution
    BufferString	getString() const;
			//!< *untranslated* but arguments evaluated

			// Translated:
    void		fillUTF8String(BufferString&) const;
			//!< *translated* with arguments inserted
    const mQtclass(QString)&		fillQString(QString&) const;
			//!< *translated* with arguments inserted
    wchar_t*		createWCharString() const;
			//!< returns new string: use 'delete []'.

protected:

    bool		isEqualTo(const uiString& oth) const;

private:

    bool			isCacheValid() const;
    const mQtclass(QString)&	getQStringInternal() const;

    friend class		uiStringData;
    mutable char*		debugstr_; //<! full string only when __debug__
    mutable uiStringData*	data_;
    mutable Threads::Lock	datalock_;
    static const uiString	emptystring_;
    static uiString		dummystring_;

    operator bool() const       = delete;
    operator int() const        = delete;
    operator float() const      = delete;
    operator double() const     = delete;

public:

    // The functions in this section are most likely not for you.

		uiString(const char* original,
			 const char* context,
			 const char* package,
			 const char* disambiguation,
			 int pluralnr);

    void	encodeStorageString(BufferString&) const;
    int		useEncodedStorageString(const char*);
		//!< returns -1 for fail, >= 0 the number of chars used
    void	getHexEncoded(BufferString&) const;
		    /*!<Encodes translated string into a const char*
				    buffer that can has only 0-9 A-F */
    bool	setFromHexEncoded(const char*);
		    //!Reads hex-data and sets the translated str.
    void	makeIndependent();
		//!< create a separate copy (with its own ref count)
    uiString&	setArg(int,const uiString&);
    void	setFrom(const mQtclass(QString)&);
		/*!<Set the translated text. No further
		    translation will be done. */
    void	addAlternateVersion(const uiString&);
    void	addLegacyVersion( const uiString& uistr )
		{ addAlternateVersion(uistr); }
    bool	translate(const mQtclass(QTranslator)&,
			  mQtclass(QString)&) const;
		//!<Returns true if the translation succeeded

    static const char*	sODLocalizationApplication() { return "od"; }

    //!<Will be deprecated (soon):

    static const uiString& emptyString()	{ return empty(); }
    bool	isSet() const			{ return !isEmpty(); }

    const OD::String&		getFullString(BufferString* =nullptr) const;

    mDeprecated			("Use fillQString")
    const mQtclass(QString)&	getQString() const;

};


typedef uiString uiWord;
typedef uiString uiPhrase;


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
mGlobal(Basic) uiString toUiString(float,char format,int precision);
mGlobal(Basic) uiString toUiString(double,char format,int precision);
mGlobal(Basic) uiString toUiString(const Coord&); //!< no decimals
mGlobal(Basic) uiString toUiString(const BufferStringSet&);
mGlobal(Basic) uiString toUiString(const MultiID&);


mGlobal(Basic) const char* toString(const uiString&);

template <class T1,class T2>
uiString toUiString( const std::pair<T1,T2>& pair )
{
    return toUiString( "%1/%2" ).arg( pair.first ).arg( pair.second );
}

mGlobal(Basic) uiString toUiString(const mQtclass(QString)&);


inline uiString& uiString::appendPhraseSameLine( const uiString& str )
{ return appendPhrase( str, CloseLine, OnSameLine ); }
inline uiString& uiString::appendAfterList( const uiString& str )
{ return appendPhrase( str, NoSep, OnNewLine ); }
inline uiString& uiString::constructWordWith( const uiString& str, bool addspc )
{ return appendPhrase( str, addspc ? Space : NoSep, OnSameLine ); }
inline uiString& uiString::preFixWord( const uiString& str )
{ const uiString kp(*this); *this = str; return postFixWord(kp); }
inline uiString& uiString::postFixWord( const uiString& str )
{ return constructWordWith( str, true ); }
inline uiString& uiString::withUnit( const char* str )
{ return withUnit( toUiString(str) ); }
inline uiString& uiString::withNumber( od_int64 nr )
{ return postFixWord( toUiString(nr) ); }
inline uiString& uiString::addMoreInfo( const uiString& str, bool newline )
{ return appendPhrase( str, MoreInfo, newline? OnNewLine : OnSameLine ); }
inline uiString& uiString::addMoreInfo( const char* str )
{ return appendPhrase( toUiString(str), MoreInfo, OnSameLine ); }
template <class T>
inline uiString& uiString::addMoreInfo( const T& t )
{ return appendPhrase( toUiString(t), MoreInfo, OnSameLine ); }


// Use when string should be revisited later
#define mToUiStringTodo(i) ::toUiString(i)
#define mFromUiStringTodo(i) ::toString(i)


/*! Adds translation of strings outside of classes for the "od" package. It
   will return a uistring where the context is "static_func_function", where
   'function' is whatever is given as the function parameter. This matches what
   is done in the filtering of the source files before lupdate is run (in
   ./dtect/update_translations.csh).
*/
mGlobal(Basic) uiString od_static_tr(const char* function_name,const char* text,
			const char* disambiguation=nullptr,int pluralnr=-1);

mGlobal(Basic) uiString getUiYesNoString(bool res);

template <class T> inline
uiString& uiString::arg( const T& var )
{
    return arg( toUiString(var) );
}

inline uiString& uiString::arg( float val, int nrdec )
{
    return arg( toUiString(val,nrdec) );
}

inline uiString& uiString::arg( double val, int nrdec )
{
    return arg( toUiString(val,nrdec) );
}


#define mGetQStr( qstr, uistring_var ) \
    QString qstr; (uistring_var).fillQString( qstr )

#define mDefIncorrectAppendPhrArgs \
    sep=='\n' ? NoSep : (sep=='\t' ? Tab : Space), \
    sep=='\n' ? OnNewLine : OnSameLine

inline uiString& uiString::appendIncorrect( const uiString& str, char sep )
{ return appendPhrase( str, mDefIncorrectAppendPhrArgs ); }
inline uiString& uiString::appendIncorrect( const char* str, char sep )
{ return appendPhrase( toUiString(str), mDefIncorrectAppendPhrArgs ); }
inline uiString& uiString::appendIncorrect( const OD::String& str, char sep )
{ return appendPhrase( toUiString(str), mDefIncorrectAppendPhrArgs ); }


//! Stored uiString's have a preamble, a size, a ':' and then data in Hex chars
//! Note that uiStrings are seldom stored, and you probably don't need to do it!
#define mStoreduiStringPreamble		"^&"


// We need the sets so often that we include it here
#include "uistringset.h"
