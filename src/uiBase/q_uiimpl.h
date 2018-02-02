#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2018
________________________________________________________________________

-*/

#include "uistring.h"
#include <QObject>

inline QString toQString( const uiString& uistr )
{
    mGetQStr( qstr, uistr );
    return qstr;
}
