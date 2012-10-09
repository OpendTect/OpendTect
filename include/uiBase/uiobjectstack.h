#ifndef uiobjectstack_h
#define uiobjectstack_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          7/9/2000
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiobj.h"

class uiGroup;
class uiObjStackBody;

mClass uiObjectStack : public uiObject
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
