#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		March 2014
________________________________________________________________________

-*/

#include <uibasemod.h>

#include "helpview.h"
#include "iopar.h"


/*!\brief A simple HelpProvider that can be extended to create a HelpProvider
    for third party plug-ins. You need a urlbase string which can refer
    to a web URL (starting with "http://") or a local file path (starting with
    "file:///"). For linking the individual HelpKeys, you can either add links
    for specific keys using the function:

	addKeyLink( const char* key, const char* link );

    or have a local file with key-value pairs like this:

	key1: link1
	key2: link2
	...........

    The links will be simply appended to urlbase. For example, look at the class
    TutHelpProvider in plugins/uiTut/uitutpi.cc
*/

mExpClass(uiBase) SimpleHelpProvider : public HelpProvider
{
protected:
			SimpleHelpProvider(const char* urlbase,
					   const char* keylinkfile=0);

    void		addKeyLink(const char* key,const char* link);

private:

    virtual bool	hasHelp(const char* arg) const;
    virtual void	provideHelp(const char* arg) const;

    BufferString	baseurl_;
    IOPar		keylinks_;

};


mExpClass(uiBase) FlareHelpProvider : public HelpProvider
{  mODTextTranslationClass(FlareHelpProvider);
public:
    static void			initClass(const char* factorykey,
					  const char* baseurl);

    static void			initODHelp();

private:
				FlareHelpProvider(const char* url);
    static HelpProvider*	createInstance();

    void			provideHelp(const char*) const;
    bool			hasHelp(const char*) const { return true; }
    BufferString		baseurl_;

public:
				//Internal to dGB
    static void			initHelpSystem(const char* context,
					       const char* path);
};


/*! HelpProvider for website pages */
mExpClass(uiBase) WebsiteHelp : public HelpProvider
{ mODTextTranslationClass(WebsiteHelp);
public:
    static void			initClass();
    static const char*		sKeyFactoryName();
    static const char*		sKeySupport();
    static const char*		sKeyVideos();
    static const char*		sKeyAttribMatrix();

private:
    static HelpProvider*	createInstance();
    bool			hasHelp(const char* arg) const;
    void			provideHelp(const char* arg) const;
};


//! HelpProvider for videos
mExpClass(uiBase) VideoProvider : public HelpProvider
{
public:
    static void			init();
    static void			initClass(const char* context,
					  const char* indexfilename);
    uiString			description(const char* arg) const;

private:
				VideoProvider(const char* idxfnm);

    static HelpProvider*	createInstance();
    bool			hasHelp(const char* arg) const;
    void			provideHelp(const char* arg) const;
    int				indexOf(const char* arg) const;

    BufferString		indexfilenm_;
    IOPar			videolinks_;
};

