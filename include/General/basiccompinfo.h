#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "namedobj.h"
#include "datachar.h"


/*!\brief Info on one component */

mExpClass(General) BasicComponentInfo : public NamedObject
{
public:
			BasicComponentInfo(const char* nm=nullptr);
			BasicComponentInfo(const BasicComponentInfo&);
			~BasicComponentInfo();

    BasicComponentInfo& operator=(const BasicComponentInfo&);
    bool		operator==(const BasicComponentInfo&) const;

    int			datatype_;
    DataCharacteristics datachar_;

    mDeprecated("Use datatype_")
    int&		datatype;
    mDeprecated("Use datachar_")
    DataCharacteristics& datachar;
};
