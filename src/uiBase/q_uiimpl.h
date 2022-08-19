#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistring.h"
#include <QObject>

inline QString toQString( const uiString& uistr )
{
    mGetQStr( qstr, uistr );
    return qstr;
}
