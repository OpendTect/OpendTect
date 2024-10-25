#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2024 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"

#include "integerid.h"


namespace Well
{

mExpClass(Well) LogID : public IntegerID<od_int32>
{
public:
    using IntegerID::IntegerID;
    static inline LogID		udf()		{ return LogID(); }
};


mExpClass(Well) D2TID : public IntegerID<od_int32>
{
public:
    using IntegerID::IntegerID;
    static inline D2TID		udf()		{ return D2TID(); }
};


mExpClass(Well) MarkerID : public IntegerID<od_int32>
{
public:
    using IntegerID::IntegerID;
    static inline MarkerID	udf()		{ return MarkerID(); }
};

} // namespace Well
