#ifndef attribfactory_h
#define attribfactory_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribfactory.h,v 1.1 2005-01-26 09:15:22 kristofer Exp $
________________________________________________________________________

-*/

#include "sets.h"

namespace Attrib
{

class Desc;
class ParamSet;
class Provider;

typedef Provider* (*ProviderCreater)(Desc&);

class ProviderFactory
{
public:
			ProviderFactory();
			~ProviderFactory();
			
			/*Interface from attribs */
    void		addParamSet( const ParamSet&, ProviderCreater );

    Provider*		create( Desc& ) const;
    ParamSet*		getParamSetCopy( Desc& ) const;

protected:
    int				indexOf( const char* ) const;
    ObjectSet<ParamSet>		paramsets;
    TypeSet<ProviderCreater>	creaters;
};

extern ProviderFactory& PF();

}; //Namespace

#endif

