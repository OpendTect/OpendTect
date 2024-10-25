#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2024 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstring.h"

class QUuid;

namespace OD
{

/*!
\brief Generates a Universally Unique Identifier (UUID)
*/

mExpClass(Basic) Uuid final
{
public:
			Uuid();
			Uuid(const char* uuidstr);
			~Uuid();

    BufferString	toString(bool withbraces) const;

    static BufferString	create(bool withbraces);

private:
    QUuid&		quuid_;
};

} // namespace OD
