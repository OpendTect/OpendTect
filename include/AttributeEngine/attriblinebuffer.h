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

class DataHolder;

/*!
\brief Attribute DataHolder Line Buffer.
*/

mExpClass(AttributeEngine) DataHolderLineBuffer
{
public:
			DataHolderLineBuffer();
    virtual		~DataHolderLineBuffer();

    DataHolder*		createDataHolder( const BinID&, int t0, int nrsamples );
    void		removeDataHolder(const BinID&);
    void		removeBefore( const BinID&, const BinID& );
    void		removeAllExcept( const BinID& );

    DataHolder*		getDataHolder( const BinID& b )
			{ return gtDataHolder(b); }
    const DataHolder*	getDataHolder( const BinID& b ) const
			{ return gtDataHolder(b); }

protected:
    void		removeInline( int lineidx );

    TypeSet<int>			inlines_;
    ObjectSet<ObjectSet<DataHolder> >	inlinedata_;
    ObjectSet<TypeSet<int> >		crossliness_;
    DataHolder*		gtDataHolder(const BinID&) const;
};

} // namespace Attrib
