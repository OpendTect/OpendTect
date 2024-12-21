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


Uuid::Uuid( bool undef )
    : quuid_(*new QUuid)
{
    if ( !undef )
	quuid_ = QUuid::createUuid();
}


Uuid::Uuid( const Uuid& oth )
    : quuid_(*new QUuid)
{
    *this = oth;
}


Uuid::~Uuid()
{
    delete &quuid_;
}


Uuid& Uuid::operator =( const Uuid& oth )
{
    quuid_ = oth.quuid_;
    return *this;
}


bool Uuid::operator ==( const Uuid& oth ) const
{
    return oth.quuid_ == quuid_;
}


bool Uuid::operator !=( const Uuid& oth ) const
{
    return !(*this == oth);
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


const Uuid& Uuid::udf()
{
    static Uuid udfid( true );
    return udfid;
}

bool Uuid::isUdf() const
{
    return quuid_.isNull();
}

} // namespace OD
