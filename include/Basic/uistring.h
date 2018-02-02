#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2014
________________________________________________________________________

-*/

#include "gendefs.h"
#include "threadlock.h"
#include "string2.h"
#include "objectset.h"

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
    bool	isPlainAscii() const;

	/*! uiStrings should only be manipulated using the arg() functions.
	    These replace the next %N (e.g. %1) with the provided argument. */

    template <class T>
    inline uiString&	arg(const T&);
    inline uiString&	arg(float,int nrdecimals);
    inline uiString&	arg(double,int nrdecimals);
    uiString&		arg(const uiString&);

	/*! appendXX() functions should be used to concatenate entire sentences.
		  You cannot just mix&match words and verbs etc.  */
    enum AppendType	{ BluntGlue, WithSpace, NewLine,
			  CloseLine, CloseAndNewLine };
    uiString&	appendPhrase(const uiString&,AppendType=CloseAndNewLine);
    uiString&	appendPhrases(const uiStringSet&,AppendType=CloseAndNewLine);
    uiString&	appendPlainText(const char*,AppendType=CloseAndNewLine);
    uiString&	appendPlainText(const OD::String&,AppendType=CloseAndNewLine);
    uiString&	constructWord( const uiString& str )
		{ return appendPhrase(str,WithSpace); }

    // TODO this is where it's used illegally, get rid of it
    inline uiString&	appendWord(const uiString&);
    inline uiString&	appendWord(const char*);
    inline uiString&	appendWord(const OD::String&);

    // TEMP-- comment out to fix a directory at a time
    inline uiString&	append( const uiString& s, bool nl=false)
			{ return appendWord(s); }
    inline uiString&	append( const char* s, bool nl=false)
			{ return appendWord(s); }
    inline uiString&	append( const OD::String& s, bool nl=false )
			{ return appendWord(s); }
    inline uiString&	addSpace(int =1)	{ return append(" "); }
    inline uiString&	addTab(int =1)		{ return append("\t"); }
    inline uiString&	addNewLine(int =1)	{ return append("\n"); }


    /*! Results: */
    BufferString		toString() const { return getFullString(); }
				//!< returns full string, *without* translation
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

    bool			operator==( const uiString&  oth ) const
				{ return isEqualTo( oth ); }
    bool			operator!=( const uiString& oth ) const
				{ return !isEqualTo( oth ); }

    BufferString		getFullString() const; // use toString() instead
				// Note the changed return type!

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


/*\brief Set of uiStrings */

mExpClass(Basic) uiStringSet
{ mODTextTranslationClass(uiStringSet);
public:

    typedef ObjectSet<uiString>::size_type	size_type;
    typedef size_type				IdxType;

			uiStringSet()				{}
			uiStringSet( const uiString& s )	{ set(s); }
			uiStringSet( const uiStringSet& oth )	{ *this = oth; }
			uiStringSet(const uiString[]);
				/*!< end array with empty string */
			~uiStringSet();
    uiStringSet&	operator =(const uiStringSet&);

    inline size_type	size() const		    { return strs_.size(); }
    inline bool		validIdx( IdxType i ) const { return strs_.validIdx(i);}
    bool		isEmpty() const		    { return strs_.isEmpty(); }
    bool		isPresent(const uiString&) const;
    IdxType		indexOf(const uiString&) const;
    uiString		get(IdxType) const;
    uiString&		operator[]( IdxType idx )	{ return *strs_[idx]; }
    const uiString&	operator[]( IdxType idx ) const	{ return *strs_[idx]; }

    void		setEmpty();
    uiStringSet&	set(const uiString&);
    uiStringSet&	set( const uiStringSet& oth )	{ return (*this=oth); }
    uiStringSet&	set(const uiRetVal&);
    uiStringSet&	add(const uiString&);
    uiStringSet&	add(const uiStringSet&);
    uiStringSet&	add(const uiRetVal&);
    uiStringSet&	append( const uiStringSet& ss )	{ return add(ss); }
    uiStringSet&	insert(IdxType,const uiString&);
    uiStringSet&	operator +=( const uiString& s ) { return add(s); }
    void		removeSingle(IdxType,bool keep_order=true);
    void		removeRange(IdxType,IdxType);

    uiString		cat(uiString::AppendType apptyp
					=uiString::CloseAndNewLine) const;
    uiStringSet		getNonEmpty() const;
    uiString		createOptionString(bool use_and=true,int maxnritems=-1,
				   bool separate_lines=false) const;
				//!< example: "option1, option2 and option3"

    void		fill(mQtclass(QStringList)&) const;
    void		sort(const bool caseinsens=true,bool asc=true);
    void		useIndexes( const IdxType* idxs );
    IdxType*		getSortIndexes(bool caseinsens,bool asc) const;

protected:

    ObjectSet<uiString>	strs_;

};


#ifndef UISTRING_FULL_SEPARATION

	typedef uiStringSet uiPhraseSet;
	typedef uiStringSet uiWordSet;

#else

	//TODO

#endif


/*\brief allows returning status and accompanying user info.

  This class helps us make sure there is always user info on errors. Therefore,
  you will find a 'setIsOK' but not OK means setting a non-empty message.

*/

mExpClass(Basic) uiRetVal
{
public:

			uiRetVal()		{}
			uiRetVal(const uiPhrase&);
			uiRetVal(const uiPhraseSet&);
			uiRetVal(const uiRetVal&);
    static uiRetVal	OK()			{ return ok_; }
    static uiRetVal	Empty()			{ return ok_; }
    uiRetVal&		operator =(const uiRetVal&);
    uiRetVal&		operator =(const uiPhrase&);
    uiRetVal&		operator =(const uiPhraseSet&);
			operator uiPhrase() const;
			operator uiPhraseSet() const;

    bool		isOK() const;
    inline bool		isEmpty() const		{ return isOK(); }
    inline bool		isError() const		{ return !isOK(); }
    bool		isMultiMessage() const;
    uiPhraseSet		messages() const;
    bool		isSingleWord(const uiWord&) const;

    uiRetVal&		setEmpty();
    inline uiRetVal&	setOK()			{ return setEmpty(); }
    uiRetVal&		insert(const uiPhrase&);
    uiRetVal&		set(const uiRetVal&);
    uiRetVal&		set(const uiPhrase&);
    uiRetVal&		set(const uiPhraseSet&);
    uiRetVal&		add(const uiRetVal&);
    uiRetVal&		add(const uiPhrase&);
    uiRetVal&		add(const uiPhraseSet&);
    uiRetVal&		setAsStatus(const uiWord&);

    BufferString	getText() const;

private:

    uiPhraseSet		msgs_;
    mutable Threads::Lock lock_;

    static const uiRetVal ok_;

};


mGlobal(Basic) bool isFinished(const uiRetVal&);
mGlobal(Basic) bool isCancelled(const uiRetVal&);


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


//TODO as said, should go away
inline uiString& uiString::appendWord( const uiString& str )
{ return appendPhrase(str,WithSpace); }
inline uiString& uiString::appendWord( const char* str )
{ return appendPhrase(toUiString(str),WithSpace); }
inline uiString& uiString::appendWord( const OD::String& str )
{ return appendPhrase(toUiString(str),WithSpace); }
