#ifndef uihelpview_h
#define uihelpview_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		March 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include <uibasemod.h>

#include "helpview.h"


mExpClass(uiBase) FlareHelpProvider : public HelpProvider
{  mODTextTranslationClass(FlareHelpProvider);
public:
    static void 		initClass(const char* factorykey,
					  const char* baseurl);

    static void 		initODHelp();

private:
				FlareHelpProvider(const char* url);
    static HelpProvider*	createInstance();

    void			provideHelp(const char*) const;
    bool			hasHelp(const char*) const { return true; }
    BufferString		baseurl_;
};


/*! HelpProvider for the developers documentation. */
mExpClass(uiBase) DevDocHelp : public HelpProvider
{
public:
    static void 		initClass();

    static const char*		sKeyFactoryName() { return "oddevdoc"; }

private:
    BufferString		getUrl() const;
    static HelpProvider*	createInstance();
    bool			hasHelp(const char* arg) const;
    void			provideHelp(const char* arg) const;
};




#endif

