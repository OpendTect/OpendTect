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

mExpClass(Basic) Uuid
{
public:
			Uuid();
			Uuid(const char* uuidstr);
			Uuid(const Uuid&);
    virtual		~Uuid();

    Uuid&		operator =(const Uuid&);
    bool		operator ==(const Uuid&) const;
    bool		operator !=(const Uuid&) const;

    bool		isUdf();

    BufferString	toString(bool withbraces) const;

    static BufferString	create(bool withbraces);
    static const Uuid&	udf();
    bool		isUdf() const;

protected:
			Uuid(bool undef);

private:

    QUuid&		quuid_;
};

} // namespace OD
