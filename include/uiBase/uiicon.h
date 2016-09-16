#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2009
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "bufstring.h"

mFDQtclass(QIcon)


/**\brief Manager for icons of different sizes.

  Icons can be created from the identifier, see odiconfile.h. It is,
  basically, the file name without extension.
*/

mExpClass(uiBase) uiIcon
{
public:
				uiIcon();
				uiIcon(const char* icon_identifier);
				uiIcon(const uiIcon&);
				~uiIcon();

    void			setIdentifier(const char*);
    bool			isEmpty() const;
    const char*			source() const;

    mQtclass(QIcon&)		qicon()		{ return qicon_; }
    const mQtclass(QIcon&)	qicon() const	{ return qicon_; }

    static const char*		None();

protected:

    mQtclass(QIcon&)		qicon_;
    BufferString		srcnm_;
};
