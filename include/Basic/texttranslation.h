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
#include "enums.h"
#include "filepath.h"

namespace TextTranslation { class TranslateMgr; class LanguageEntry; }
mFDQtclass(QTranslator)
mFDQtclass(QLocale)
mFDQtclass(QString)


/*! Global access to the manager keeping track of translations. */
mGlobal(Basic) TextTranslation::TranslateMgr& TrMgr();


namespace TextTranslation
{

/*! keeps track of translations, either from the release or ones you provide
    yourself.

  Keys are:
  * ISO locale key ("en", "zh_CN", ...). The actual 'language'
  * Package key ("od", "dgb", ...) start of the .qm filename, like od_zh_CN.qm

  Every uiString carries the package key with it. For OD, you can simply
  make this happen by using the mODTextTranslationClass(classname) macro. For
  your own package, create a similar macro. If you have your own translations
  for your own plugins, think of a short key for your 'package' (say 'mypkg').
  Then:
  * create a similar macro to mODTextTranslationClass and use it
  * use tr() in your code, and 'lupdate' to harvest the original strings
  * when the .qm files are created, install those in one of:
    - <home_dir>/.od/translations/
    - <installation_super_dir>/data/translations/
    - <release_dir>/data/translations/
  The filenames *have* to be like: mypkg_zh_CN.qm (i.e. using Qt-style naming).
 */

mExpClass(Basic) TranslateMgr : public CallBacker
{ mODTextTranslationClass(TranslateMgr);
public:
    enum			Language { English, Chinese, Spanish };
				mDeclareEnumUtils(Language);
				
				TranslateMgr();
				~TranslateMgr();

    int				nrSupportedLanguages() const;
    int				currentLanguageIdx() const;
    uiRetVal			setLanguage(int);
    static int			cDefaultLocaleIdx()	{ return 0; }

    uiString			languageUserName(int) const;
    BufferString		localeKey(int) const;
    uiRetVal			setLanguageByLocaleKey(const char*);
    void			storeToUserSettings();

    Notifier<TranslateMgr>	languageChange;
    int				changeCount() const	{ return dirtycount_; }
    void			touch()			{ handleChange(); }

    const mQtclass(QTranslator)* getQTranslator(const char* packagekey) const;
    const mQtclass(QLocale)*	getQLocale() const;

protected:

    const File::Path		userlocdir_;
    const File::Path		applocdir_;
    const File::Path		instlocdir_;

    friend class		LanguageEntry;

    BufferStringSet		pkgnms_;
    ObjectSet<LanguageEntry>	langentries_;
    int				curentryidx_;
    int				dirtycount_;

    void			findLocales(const File::Path&,BufferStringSet&);
    void			addEntry(LanguageEntry*);
    void			handleChange();
    LanguageEntry*		getEntry4Locale(const char*) const;

public:

    static const char*	sLocalizationSubDirName()   { return "translations"; }
    static const char*	sPackageLanguageSeparator() { return "_"; }

    void			addLanguage(LanguageEntry*);
				//!< Only needed if you go outside the
				//!< 'normal' loading procedures
    void			reInit();
				//!< Should never be necessary
    void			addLanguages(const char* dirnm,
					     const char* pkgky);

    BufferString		locDir( bool usr ) const
				{ return (usr?userlocdir_:instlocdir_)
					 .fullPath(); }

};


/*! Data around one .qm file, hence one QTranslator instance. */

mExpClass(Basic) TranslatorData
{
public:
				~TranslatorData();

    BufferString		pkgnm_;
    BufferString		filename_;
    mQtclass(QTranslator)*	qtranslator_	= 0;
};


/*! holds the translations for one language. The key is the locale code,
  (such as en-us and cn-cn). Usually, you do not deal with this class - unless
  you load your own languages (i.e. if you do not install .qm files in
  data/translations but manage everything yourself). */

mExpClass(Basic) LanguageEntry
{ mODTextTranslationClass(TextTranslation::LanguageEntry);
public:

				LanguageEntry(const char* localekey,
					      const BufferStringSet& pkgnms);
				LanguageEntry(const char* filenm,
					      const char* localekey);
				~LanguageEntry();

    const mQtclass(QString)&	qName() const		{ return *qlangname_;}
    const mQtclass(QLocale)&	qLocale() const		{ return *qlocale_; }
    BufferString		localeKey() const	{ return localekey_; }

    bool			isLoaded() const;
    bool			load();

    const mQtclass(QTranslator)* getQTranslator(const char* pkgnm) const;

protected:

    BufferString		localekey_;
    bool			addtoentry_;

    void			addData4Dir(const File::Path&,
					    const BufferStringSet&);
    TranslatorData*		getData4Pkg(const char*) const;
    void			setLangName(const char* qmfnm);

public:
    // public section to allow external language loading

    mQtclass(QString)*		qlangname_; // always allocated
    mQtclass(QLocale)*		qlocale_;   // always allocated

    ObjectSet<TranslatorData>	trdata_;
    mQtclass(QTranslator)*	qt_transl_;

    void			addData(const char* qmfnm,const char* pkgnm);
    bool			addToEntry() { return addtoentry_; }

};

} // namespace TextTranslation
