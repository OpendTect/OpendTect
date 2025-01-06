#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "oduuid.h"

namespace OD
{

/*!
\brief Unique DataSet Identifier, uniquely identifying a dataset regardless of:
	its (user) name,
	file implementation,
	database ID (MultiID/DBKey),
	project location
*/

mExpClass(General) DataSetKey final : public Uuid
{
public:
			DataSetKey();
			DataSetKey(const char* dsidstr);
			~DataSetKey();

    void		fillPar(IOPar&) const;

    static BufferString create(bool withbraces);
    static DataSetKey	getFrom(const IOPar&);
    static const DataSetKey& udf();

    static const char*	sKeyID();

private:
			DataSetKey(bool undef);
};

} // namespace OD
