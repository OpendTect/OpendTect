#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2014
________________________________________________________________________

-*/

#include "string2.h"
#include "threadlock.h"
#include "bufstring.h"

class uiStringData;
class uiStringSet;
class uiRetVal;
namespace Pos { class IdxPair; }

mFDQtclass( QString );
mFDQtclass( QStringList );
mFDQtclass( QTranslator );


#define mTextTranslationClass(clss,pkgkey) \
private: \
 static inline uiString tr( const char* text, const char* disambiguation = 0,  \
 int pluralnr=-1 ) \
 { return uiString( text, #clss, pkgkey, disambiguation, pluralnr ); } \

#define mODTextTranslationClass(clss) \
mTextTranslationClass( clss, uiString::sODLocalizationApplication() )

//! Stored uiString's have a preamble, a size, a ':' and then data in Hex chars
#define mStoreduiStringPreamble		"^&"


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

  -# Declare the mTextTranslationClass(classname,packagekey) in its class
     definition. The packagekey is a string that identifies your software
     package. OpendTect's internal classes use the "od" package string, and
     can for short use the mODTextTranslationClass macro.
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
     data/translations/<packagekey>_<lang>_<country>.ts in the release. For
     example, a localization of OpendTect to modern Chinese would be
     saved as od_cn-cn.qm.
 */


mExpClass(Basic) uiString
{ mODTextTranslationClass(uiString);
public:
		uiString();
		uiString(const uiString&);	//!< no copy, ref counted
		~uiString();

    uiString&	set(const char*);
    uiString&	importFrom(const char*);
    bool	isEmpty() const;
    void	setEmpty();
    uiString&	toLower( bool yn=true ); //!< Applied before argument subst.
    uiString&	toUpper( bool yn=true )		{ return toLower(!yn); }
    bool	operator!() const		{ return isEmpty(); }

    uiString&	operator=(const uiString&);	//!< no copy, ref counted
    bool	operator>(const uiString& b) const;
    bool	operator<(const uiString& b) const;
    int		size() const;
    static const uiString& emptyString()	{ return emptystring_; }
    static uiString& dummyString()		{ return dummystring_; }
    bool	isPlainAscii() const;

	/*! uiStrings should only be manipulated using the arg() functions.
	    These replace the next %N (e.g. %1) with the provided argument. */

    template <class T>
    inline uiString&	arg(const T&);
    inline uiString&	arg(float,int nrdecimals);
    inline uiString&	arg(double,int nrdecimals);
    uiString&		arg(const uiString&);

			/*! appendXX() functions should be used to concatenate
			    entire sentences. You cannot just mix&match words
			    and verbs etc.  */
#   define		muiStringAppendDefArgs \
			    SeparType septyp=uiString::CloseLine, \
			    AppendType apptyp=uiString::AddNewLine
    enum SeparType	{ Empty, CloseLine, Space, Tab, Comma, MoreInfo };
    enum AppendType	{ SeparatorOnly, AddNewLine };
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
    uiString&		addMoreInfo(const uiString&,bool newline=true);
			//!< adds a colon first
    uiString&		appendEmptyLine()
			{ return appendPlainText("\n\n"); }

    inline uiString&	appendIncorrect(const uiString&,char sep=' ');
    inline uiString&	appendIncorrect(const char*,char sep=' ');
    inline uiString&	appendIncorrect(const OD::String&,char sep=' ');

    // TEMP-- comment out to fix a directory at a time
    inline uiString&	append( const uiString& s, bool nl=false)
			{ return appendIncorrect(s,nl?'\n':' '); }
    inline uiString&	append( const char* s, bool nl=false)
			{ return appendIncorrect(s,nl?'\n':' '); }
    inline uiString&	append( const OD::String& s, bool nl=false )
			{ return appendIncorrect(s,nl?'\n':' '); }
    inline uiString&	addSpace(int =1)	{ return append(" "); }
    inline uiString&	addTab(int =1)		{ return append("\t"); }
    inline uiString&	addNewLine(int =1)	{ return append("\n"); }

			// UNtranslated:
    const char*		getOriginalString() const;
			//!< *untranslated*, no argument substitution
    BufferString	toString() const;
			//!< *untranslated* but arguments evaluated

			// Translated:
    void		fillUTF8String(BufferString&) const;
			//!< *translated* with arguments inserted
    void		fillQString(QString&) const;
			//!< *translated* with arguments inserted
    wchar_t*		createWCharString() const;
			//!< returns new string: use 'delete []'.
private:

    inline			operator bool() const	{ return !isEmpty(); }
    bool			isCacheValid() const;
    const mQtclass(QString)&	getQStringInternal() const;

    friend class		uiStringData;

    char*			debugstr_;
				/*<!< Contains full string() for easy debugging
				      Only filled in in debug builds. */

    mutable uiStringData*	data_;
    mutable Threads::Lock	datalock_;	//!< Protects data_ variable
    static const uiString	emptystring_;
    static uiString		dummystring_;

    bool			operator==( const uiString&  oth ) const
				{ return isEqualTo( oth ); }
    bool			operator!=( const uiString& oth ) const
				{ return !isEqualTo( oth ); }

    void			getFullString(BufferString&) const; // toString

public:

    bool	isEqualTo(const uiString& oth) const;
		//!< use only if unavoidable

    void	encodeStorageString(BufferString&) const;
    int		useEncodedStorageString(const char*);
		//!< returns -1 for fail, >= 0 the number of chars used

    void	getHexEncoded(BufferString&) const;
		    /*!<Encodes translated string into a const char*
				    buffer that can has only 0-9 A-F */
    bool	setFromHexEncoded(const char*);
		    //!Reads hex-data and sets the translated str.

    static const char*	sODLocalizationApplication() { return "od"; }

    void	makeIndependent();
		//!< create a separate copy (with its own ref count

		uiString(const char* original,
			 const char* context,
			 const char* package,
			 const char* disambiguation,
			 int pluralnr);
    void	setFrom(const mQtclass(QString)&);
		/*!<Set the translated text. No further
		    translation will be done. */
    void	addAlternateVersion(const uiString&);

    bool	translate(const mQtclass(QTranslator)&,
			  mQtclass(QString)&) const;
		//!<Returns true if the translation succeeded

    static uiString getOrderString(int);
		//Returns 1st, 2nd, 3rd

};


#ifndef UISTRING_FULL_SEPARATION

	typedef uiString uiWord;
	typedef uiString uiPhrase;

#else

//TODO make this better

mExpClass(Basic) uiWord : public uiString
{ mODTextTranslationClass(uiWord);
public:
		uiWord();
		uiWord(const uiWord&);
    explicit	uiWord(const uiString&);
		~uiWord();

};


mExpClass(Basic) uiPhrase : public uiString
{ mODTextTranslationClass(uiPhrase);
public:
		uiPhrase();
		uiPhrase(const uiPhrase&);
    explicit	uiPhrase(const uiString&);
		~uiWord();

    uiString&	set(const UserNameString&)	= delete;

};

#endif


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
mGlobal(Basic) uiString toUiString(const Coord&); //!< no decimals


mGlobal(Basic) inline BufferString toString( const uiString& uis )
{
    return uis.toString();
}

template <class T1,class T2>
uiString toUiString( const std::pair<T1,T2>& pair )
{
    return toUiString( "%1/%2" ).arg( pair.first ).arg( pair.second );
}

inline uiString& uiString::appendPhraseSameLine( const uiString& str )
{ return appendPhrase( str, CloseLine, SeparatorOnly ); }
inline uiString& uiString::appendAfterList( const uiString& str )
{ return appendPhrase( str, Empty, AddNewLine ); }
inline uiString& uiString::constructWordWith( const uiString& str, bool addspc )
{ return appendPhrase( str, addspc ? Space : Empty, SeparatorOnly ); }
inline uiString& uiString::preFixWord( const uiString& str )
{ const uiString kp(*this); *this = str; return postFixWord(kp); }
inline uiString& uiString::postFixWord( const uiString& str )
{ return constructWordWith( str, true ); }
inline uiString& uiString::addMoreInfo( const uiString& str, bool newline )
{ return appendPhrase( str, MoreInfo, newline?AddNewLine:SeparatorOnly ); }


// Use when string should be revisited later
#define mToUiStringTodo(i) ::toUiString(i)
#define mFromUiStringTodo(i) ::toString(i)


/*!Adds translation of strings outside of classes for the "od" package. It
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
mGlobal(Basic) uiString od_static_tr(const char* context,const char* text,
				const char* disambiguation=0,int pluralnr=-1);

mGlobal(Basic) uiWord getUiYesNoWord(bool);

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
    sep=='\n' ? Empty : (sep=='\t' ? Tab : Space), \
    sep=='\n' ? AddNewLine : SeparatorOnly

inline uiString& uiString::appendIncorrect( const uiString& str, char sep )
{ return appendPhrase( str, mDefIncorrectAppendPhrArgs ); }
inline uiString& uiString::appendIncorrect( const char* str, char sep )
{ return appendPhrase( toUiString(str), mDefIncorrectAppendPhrArgs ); }
inline uiString& uiString::appendIncorrect( const OD::String& str, char sep )
{ return appendPhrase( toUiString(str), mDefIncorrectAppendPhrArgs ); }


#include "uistringset.h"
