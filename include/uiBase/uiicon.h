#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
