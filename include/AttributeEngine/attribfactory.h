#ifndef attribfactory_h
#define attribfactory_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id$
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "sets.h"

namespace Attrib
{

class Desc;
class Provider;

typedef Provider* (*ProviderCreater)(Desc&);

/*!
  \ingroup AttributeEngine
  \brief Factory for attribute providers.
*/

mExpClass(AttributeEngine) ProviderFactory
{
public:
			ProviderFactory();
			~ProviderFactory();
			
			/*Interface from attribs' initClass() */
    void		addDesc( Desc*, ProviderCreater );

    int			size() const			{ return descs_.size();}
    const Desc&		getDesc( int idx ) const	{ return *descs_[idx]; }
    const Desc*		getDesc(const char*) const;

    Provider*		create( Desc& ) const;
    Desc*		createDescCopy( const char* nm ) const;
    void		updateAllDescsDefaults();

protected:

    int			indexOf( const char* ) const;
    ObjectSet<Desc>	descs_;
    TypeSet<ProviderCreater> creaters_;

};

mGlobal(AttributeEngine) extern ProviderFactory& PF();

}; //Namespace

#endif


