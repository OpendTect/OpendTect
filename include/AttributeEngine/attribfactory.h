#ifndef attribfactory_h
#define attribfactory_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribfactory.h,v 1.3 2005-02-01 14:05:34 kristofer Exp $
________________________________________________________________________

-*/

#include "sets.h"

namespace Attrib
{

class Desc;
class Provider;

typedef Provider* (*ProviderCreater)(Desc&);

class ProviderFactory
{
public:
			ProviderFactory();
			~ProviderFactory();
			
			/*Interface from attribs */
    void		addDesc( Desc*, ProviderCreater );

    Provider*		create( Desc& ) const;
    Desc*		createDescCopy( const char* nm ) const;

protected:
    int				indexOf( const char* ) const;
    ObjectSet<Desc>		descs;
    TypeSet<ProviderCreater>	creaters;
};

extern ProviderFactory& PF();

}; //Namespace

#endif

