#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attributeenginemod.h"

#include "attribdesc.h"
#include "attribprovider.h"
#include "sets.h"

namespace Attrib
{

typedef Provider* (*ProviderCreater)(Desc&);

/*!
\brief Factory for attribute providers.
*/

mExpClass(AttributeEngine) ProviderFactory final
{
public:
			ProviderFactory();
			~ProviderFactory();

			/*Interface from attribs' initClass() */
    void		addDesc(Desc*,ProviderCreater);
    void		remove(const char* attrnm);

    int			size() const			{ return descs_.size();}
    const Desc&		getDesc( int idx ) const	{ return *descs_[idx]; }
    const Desc*		getDesc(const char*) const;

    RefMan<Provider>	create(Desc&) const;
    RefMan<Desc>	createDescCopy(const char* nm) const;
    void		updateAllDescsDefaults();

protected:

    int			indexOf( const char* ) const;
    RefObjectSet<Desc>	descs_;
    TypeSet<ProviderCreater> creaters_;

};

mGlobal(AttributeEngine) extern ProviderFactory& PF();

} // namespace Attrib
