#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
\brief Factory for attribute providers.
*/

mExpClass(AttributeEngine) ProviderFactory
{
public:
			ProviderFactory();
			~ProviderFactory();

			/*Interface from attribs' initClass() */
    void		addDesc( Desc*, ProviderCreater );
    void		remove(const char* attrnm);

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

} // namespace Attrib
