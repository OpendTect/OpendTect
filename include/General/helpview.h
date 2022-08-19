#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "bufstring.h"
#include "factory.h"

class HelpKey;


/*!
The generalization of a provider that can provide help in some way. Each
 provider is identified by an providername, and an argument (encapsulated in
 an HelpKey). The providername is used to create a HelpProvider, and the
 argument is given to it to produce a help.

*/


mExpClass(General) HelpProvider
{
public:
			mDefineFactoryInClass(HelpProvider,factory);
    virtual		~HelpProvider() 			{}

    static void 	provideHelp(const HelpKey&);
    static bool 	hasHelp(const HelpKey&);
    static uiString	description(const HelpKey&);

private:
    virtual bool	hasHelp(const char* arg) const		= 0;
    virtual void	provideHelp(const char* arg) const	= 0;
    virtual uiString	description(const char* arg) const;
};


/* Key to contain both a helpprovidername and an argument */
mExpClass(General) HelpKey
{
public:
			HelpKey(const char* providername,const char* arg)
			    : providername_( providername ), argument_(arg) {}
			HelpKey() : providername_( 0 )		{}

    bool		operator==(const HelpKey&) const;
    static HelpKey	emptyHelpKey();
    bool		isEmpty() const;

    BufferString 	providername_;
    BufferString 	argument_;

			// Legacy
			HelpKey(const char* arg)
			    : providername_("od")
			    , argument_( arg )
			{}
};

#define mODHelpKey( arg ) HelpKey( "od", ::toString(arg) )
#define mODVideoKey( arg ) HelpKey( "odvideo", ::toString(arg) )
