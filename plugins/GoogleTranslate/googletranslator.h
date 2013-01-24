#ifndef googletranslator_h
#define googletranslator_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "googletranslatemod.h"
#include "texttranslator.h"

#include "bufstring.h"

class ODHttp;

mClass(GoogleTranslate) GoogleTranslator : public TextTranslator
{
public:
			GoogleTranslator();
			~GoogleTranslator();

    void		enable();
    void		disable();
    bool		enabled() const;

    int			nrSupportedLanguages() const;
    const wchar_t*	getLanguageUserName(int) const;
    const char*		getLanguageName(int) const;
    bool		supportsLanguage(const char*) const;

    bool		setToLanguage(const char*);
    const char*		getToLanguage() const;

    void		setAPIKey(const char* key);

    int			translate(const char*);
    const wchar_t*	get() const;

    const char*		getIcon() const;

    ODHttp&		http()		{ return odhttp_; }

protected:

    void		init();
    void		add(const wchar_t*,const char*,const char*);
    void		createUrl(BufferString&,const char*);
    void		readyCB(CallBacker*);
    void		disConnCB(CallBacker*);
    void		messageCB(CallBacker*);

    ODHttp&		odhttp_;
    mutable wchar_t*	translation_;

	mStruct(GoogleTranslate) LanguageInfo
	{
				LanguageInfo( const wchar_t* unm,
					      const char* nm, const char* code )
				    : name_(nm), code_(code)
				{
				    username_ = new wchar_t [ wcslen(unm)+1 ];
				    wcscpy(username_,unm);
				    username_[ wcslen(unm) ] = L'\0';
				}

				~LanguageInfo()	    { delete [] username_; }

	    wchar_t*		username_;
	    BufferString	name_;
	    BufferString	code_;
	};

    ObjectSet<LanguageInfo>	infos_;
    LanguageInfo*		tolanguage_;

    BufferString		apikey_;
};

#endif

