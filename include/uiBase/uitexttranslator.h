#ifndef uitexttranslator_h
#define uitexttranslator_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id$
________________________________________________________________________

-*/


#include "uibasemod.h"
#include "callback.h"
#include "uistring.h"


mExpClass(uiBase) TextTranslator : public CallBacker
{
public:

    virtual void		enable()			= 0;
    virtual void		disable()			= 0;
    virtual bool		enabled() const			= 0;

    virtual int			nrSupportedLanguages() const	= 0;
    virtual const wchar_t*	getLanguageUserName(int) const	= 0;
    virtual const char*		getLanguageName(int) const	= 0;
    virtual bool		supportsLanguage(const char*) const	= 0;

    virtual bool		setToLanguage(const char*)	= 0;
    virtual const char*		getToLanguage() const		= 0;

    virtual int			translate(const char*)		= 0;
    virtual const wchar_t*	get() const			= 0;

    virtual const char*		getIcon() const			{ return ""; }

    CNotifier<TextTranslator,int>	ready;
    Notifier<TextTranslator>		message;

protected:

				TextTranslator()
				    : ready(this), message(this)	{}

};


mExpClass(uiBase) TextTranslateMgr : public CallBacker
{
public:
				TextTranslateMgr()
				    : translator_(0)
				    , dirtycount_(0)
				    , languageChange(this)
				{}
				~TextTranslateMgr()	{ delete translator_; }



    int 			nrSupportedLanguages() const;
    uiString			getLanguageUserName(int) const;
    const char* 		getLanguageName(int) const;
    void			setLanguage(const char*);

    Notifier<TextTranslateMgr>	languageChange;

    int 			dirtyCount() const	{ return dirtycount_; }
				//Increased every time language is changed


protected:

    TextTranslator*			translator_;
    Threads::Atomic<int>		dirtycount_;


public: 			//Old stuff will be removed.
    void			setTranslator( TextTranslator* ttr )
    { translator_ = ttr; }
    TextTranslator*		tr()			{ return translator_; }
};


mGlobal(uiBase) TextTranslateMgr& TrMgr();

#endif

