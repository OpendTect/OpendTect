#ifndef attribfactory_h
#define attribfactory_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribfactory.h,v 1.4 2005-05-09 14:40:01 cvshelene Exp $
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
			
			/*Interface from attribs' initClass() */
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

