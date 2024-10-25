/*+
________________________________________________________________________

 Copyright:	(C) 1995-2024 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "oduuid.h"

#include <QUuid>

namespace OD
{

Uuid::Uuid()
    : quuid_(*new QUuid)
{
    quuid_ = QUuid::createUuid();
}


Uuid::Uuid( const char* uuidstr )
    : quuid_(*new QUuid(uuidstr))
{
}


Uuid::~Uuid()
{
    delete &quuid_;
}


BufferString Uuid::toString( bool withbraces ) const
{
    return quuid_.toString( withbraces ? QUuid::StringFormat::WithBraces
				       : QUuid::StringFormat::WithoutBraces );
}


BufferString Uuid::create( bool withbraces )
{
    const QUuid quuid = QUuid::createUuid();
    if ( quuid.isNull() )
	return BufferString::empty();

    return quuid.toString( withbraces ? QUuid::StringFormat::WithBraces
				      : QUuid::StringFormat::WithoutBraces );
}

} // namespace OD
