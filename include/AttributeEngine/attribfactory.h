#ifndef attribfactory_h
#define attribfactory_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribfactory.h,v 1.2 2005-01-28 16:30:41 kristofer Exp $
________________________________________________________________________

-*/

#include "sets.h"

namespace Attrib
{

class Desc;
class Parser;
class Provider;

typedef Provider* (*ProviderCreater)(Desc&);

class ProviderFactory
{
public:
			ProviderFactory();
			~ProviderFactory();
			
			/*Interface from attribs */
    void		addParser( Parser*, ProviderCreater );

    Provider*		create( Desc& ) const;
    Parser*		createParserCopy( Desc& ) const;

protected:
    int				indexOf( const char* ) const;
    ObjectSet<Parser>		paramsets;
    TypeSet<ProviderCreater>	creaters;
};

extern ProviderFactory& PF();

}; //Namespace

#endif

