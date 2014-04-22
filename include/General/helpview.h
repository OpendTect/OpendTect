#ifndef helpview_h
#define helpview_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		18-8-2000
 RCS:		$Id$
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


private:
    virtual bool	hasHelp(const char* arg) const		= 0;
    virtual void	provideHelp(const char* arg) const	= 0;
};


/* Key to contain both a helpprovidername and an argument */
mExpClass(General) HelpKey
{
public:
			HelpKey(const char* providername,const char* arg)
			    : providername_( providername ), argument_(arg) {}
                        HelpKey() : providername_( 0 )                      {}

    static HelpKey      emptyHelpKey();
    bool		isEmpty() const;

    const char* 	providername_;
    BufferString 	argument_;


			//This constructor is for legacy stuff. Remove.
			HelpKey(const char* arg=0)
			    : providername_("od")
			    , argument_( arg )
			{}
};

#define mODHelpKey( arg ) HelpKey( "od", toString(arg) )


#endif

