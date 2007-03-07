#ifndef uiobjectstack_h
#define uiobjectstack_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          7/9/2000
 RCS:           $Id: uiobjectstack.h,v 1.1 2007-03-07 17:49:38 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiobj.h"

class uiGroup;
class uiObjStackBody;

class uiObjectStack : public uiObject
{
public:

                        uiObjectStack(uiParent*,const char*);

    int			addObject(uiObject*);
    int			addGroup(uiGroup*);
    void		setCurrentObject(int);
    int			size() const;

private:

    uiObjStackBody*	body_;
    uiObjStackBody&	mkbody(uiParent*,const char*);

};

#endif
