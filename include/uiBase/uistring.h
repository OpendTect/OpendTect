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

#include "objectset.h"

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
			uiString(const char* original = 0);
			/*!<String is assumed to stay alive as it will be used
			    for the originalstring_. */
			uiString(const FixedString&);
			/*!<Underlying const char is assumed to stay alive and
			    will be used for the originalstring_ */
			uiString(const BufferString&);
			uiString(const QString&,
				 const char* originalstring=0);
			~uiString();

    bool		isEmpty() const;

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

    const mQtclass(QString&)	getQtString() const;
    const char*			getOriginalString() const;

private:

    void			addArgument(uiString*);

    ObjectSet<uiString>		arguments_;

    mQtclass(QString)*		qstring_;
    const char*			originalstring_;
    const char*			translationcontext_;
    const char*			translationdisambiguation_;
    int				translationpluralnumber_;

public: // Expert usage
    void		setTranslationContext(const char*);
			//!<\note String is assumed to remain in mem.
    void		setTranslationDisambiguation(const char*);
			//!<\note String is assumed to remain in mem
    void		setTranslationPluralNr(int);
			//!<If translation depends on plural, give current nr

    void		update(bool translate=true,
				bool replaceargs=true);
			/*!<Updates internal qstrig from originalstring_
			 and the other translation settings using the
			 current language. Call is normally only needed
			 if global translation language has changed. */
			uiString(const char* text,
				 const char* context,
				 const char* disambiguation,
				 int pluralnr);


};

#define mTextTranslationClass(clss) \
private: \
 static inline uiString tr( const char* text, const char* disambiguation = 0,  \
 int pluralnr=-1 ) \
 { return uiString( text, #clss, disambiguation, pluralnr ); }


#endif

