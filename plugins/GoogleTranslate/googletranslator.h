#ifndef googletranslator_h
#define googletranslator_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2010
 RCS:		$Id: googletranslator.h,v 1.4 2010-10-26 06:41:37 cvsnanne Exp $
________________________________________________________________________

-*/

#include "texttranslator.h"

#include "bufstring.h"
#include <string>


class ODHttp;

class GoogleTranslator : public TextTranslator
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

    int			translate(const char*);
    const wchar_t*	get() const;

    ODHttp&		http()		{ return odhttp_; }

protected:

    void		init();
    void		readyCB(CallBacker*);
    void		disConnCB(CallBacker*);
    void		messageCB(CallBacker*);

    ODHttp&		odhttp_;
    mutable std::wstring translation_;

	mStruct LanguageInfo
	{
				LanguageInfo( const wchar_t* unm,
					      const char* nm, const char* code )
				    : username_(unm), name_(nm), code_(code) {}

	    std::wstring	username_;
	    BufferString	name_;
	    BufferString	code_;
	};

    ObjectSet<LanguageInfo>	infos_;
    LanguageInfo*		tolanguage_;
};

#endif
