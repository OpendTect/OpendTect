#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
________________________________________________________________________

-*/


#include "basicmod.h"
#include "notify.h"
#include "uistring.h"
#include "bufstringset.h"

mFDQtclass(QTranslator)
mFDQtclass(QLocale)
mFDQtclass(QString)

class TextTranslatorLanguage;
namespace File { class Path; }

/*! Manager keeping track of translations.
 */

mExpClass(Basic) TextTranslateMgr : public CallBacker
{ mODTextTranslationClass(TextTranslateMgr);
public:
				TextTranslateMgr();
				~TextTranslateMgr();

    int				nrSupportedLanguages() const;
    int				currentLanguage() const;
    uiRetVal			setLanguage(int);

    uiString			getLanguageUserName(int) const;
    BufferString		getLocaleKey(int) const;
					//!< "en-us", "cn-cn", ....
    uiRetVal			setLanguageByLocaleKey(const char*);
    void			storeToUserSettings();

    Notifier<TextTranslateMgr>	languageChange;
				/*!<Triggers both on new languages and changed
				    languages. */

    int				changeCount() const	{ return dirtycount_; }
				//Increased every time language is changed

    const mQtclass(QTranslator)* getQTranslator(const char* appl) const;
    const mQtclass(QLocale)*	getQLocale() const;

protected:

    friend class			TextTranslatorLanguage;

    void				loadUSEnglish();

    int					dirtycount_;
    ObjectSet<TextTranslatorLanguage>	languages_;
    int					currentlanguageidx_;

public:

    static void			GetLocalizationDir(File::Path&);
    static char			cApplicationEnd() { return '_'; }

    bool			addLanguage(TextTranslatorLanguage*);
				//!<Returns false if it was not added.
};


/*!Holds the translation for one language. Each language has its own locale
   code, (such as en-us) */
mExpClass(Basic) TextTranslatorLanguage : public RefCount::Referenced
{ mODTextTranslationClass(TextTranslatorLanguage);
public:
					TextTranslatorLanguage(
					    const char*vlocalename);

    const mQtclass(QString)&		getLanguageName() const;
    const mQtclass(QLocale)&		getLanguageLocale() const;
    BufferString			getLocaleKey() const;

    bool				addFile(const char* filename);

    bool				load();

    const mQtclass(QTranslator)*	getTranslator(const char* appl) const;

protected:
                                        ~TextTranslatorLanguage();

    bool				loaded_;
    BufferString			localekey_;	// part of filename
    BufferString			localename_;	// Qt key
    mQtclass(QString)*			languagename_;
    mQtclass(QLocale)*			locale_;

    BufferStringSet			filenames_;

    ObjectSet<QTranslator>		translators_;
    BufferStringSet			applications_;
};

mGlobal(Basic) TextTranslateMgr& TrMgr();
namespace OD { mGlobal(Basic) void loadLocalization(); }
