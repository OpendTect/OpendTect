#ifndef uiattrfact_h
#define uiattrfact_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: uiattribfactory.h,v 1.1 2006-09-11 06:53:42 cvsnanne Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"

class uiAttrDescEd;
class uiParent;

/*! \brief Factory for attrib editors.  */


typedef uiAttrDescEd* (*CreateFunc)(uiParent*);

class uiAttributeFactory
{
public:
    void		add(const char* displaynm,CreateFunc fc);
    uiAttrDescEd*	create(uiParent*,const char* nm);

    int			size() const;
    const char*		getDisplayName(int) const;
    bool		hasAttribute(const char* dispnm) const;

protected:
    BufferStringSet	displaynames_;
    TypeSet<CreateFunc>	funcs_;
};

uiAttributeFactory& uiAF();


#endif
