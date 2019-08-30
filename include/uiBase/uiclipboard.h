#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Aug 2014
________________________________________________________________________

Wrapper class around QClipboard

-*/

#include "uibasemod.h"
#include "commondefs.h"

mFDQtclass(QImage);
mFDQtclass(QPixmap);
mFDQtclass(QString);

class uiString;
class BufferString;
namespace OD { class RGBImage; }


/*!Wrapper class around the QClipboard */

mExpClass(uiBase) uiClipboard
{
public:
    static void		getText(uiString&);
    static void		getText(BufferString&);
    static void		setText(const uiString&);
    static QString	getText();

    static void		setImage(const QImage&);
    static void		setImage(const OD::RGBImage&);
};
