#ifndef uiicons_h
#define uiicons_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "bufstring.h"

mFDQtclass(QIcon)

class uiPixmap;

/**\brief Manager for icons of different sizes.

  Icons can be created from the identifier, see odiconfile.h. It is,
  basically, the file name without extension.
*/

mExpClass(uiBase) uiIcon
{
public:
				uiIcon();
				uiIcon(const char* icon_identifier);
				uiIcon(const uiPixmap&);
				uiIcon(const uiIcon&);
				~uiIcon();

    void			setIdentifier(const char*);
    bool			isEmpty() const;
    const char*			source() const;

    mQtclass(QIcon&)		qicon()		{ return qicon_; }
    const mQtclass(QIcon&)	qicon() const	{ return qicon_; }

    static const char*		save();
    static const char*		saveAs();
    static const char*		openObject();
    static const char*		newObject();
    static const char*		removeObject();
    static const char*		None();

protected:

    mQtclass(QIcon&)		qicon_;
    BufferString		srcnm_;
};

#endif
