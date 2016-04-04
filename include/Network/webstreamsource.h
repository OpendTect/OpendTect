#ifndef webstreamsource_h
#define webstreamsource_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/


#include "networkmod.h"
#include "strmprov.h"
#include "uistring.h"


/*!\brief provides streams based on web services */

mExpClass(Network) WebStreamSource : public StreamProvider::StreamSource
{ mODTextTranslationClass(WebStreamSource);

public:

			WebStreamSource();

    virtual const char*	name() const	{ return "Web"; }
    virtual bool	supports( StreamSource::Type typ ) const
			{ return typ != StreamSource::Edit; }
    virtual bool	canHandle(const char*) const;
    virtual bool	fill(StreamData&,StreamSource::Type) const;

    static bool		willHandle(const char*);
    static void		initClass();

};


#endif
