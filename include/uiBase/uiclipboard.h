#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "commondefs.h"

mFDQtclass(QImage);
mFDQtclass(QPixmap);
class uiString;


/*!Wrapper class around the QClipboard */

mExpClass(uiBase) uiClipboard
{
public:
    static void		setText(const uiString&);
    static void		setText(const char*);
    static void		setImage(const QImage&);
};
