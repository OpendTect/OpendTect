#ifndef uilabel_h
#define uilabel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          7/9/2000
 RCS:           $Id: uilabel.h,v 1.6 2004-03-02 13:01:07 nanne Exp $
________________________________________________________________________

-*/

#include "uiobj.h"

class uiLabelBody;

class uiLabel : public uiObject
{
public:

                        uiLabel(uiParent*,const char*);
                        uiLabel(uiParent*,const char*,uiObject*);
                        uiLabel(uiParent*,const char*,uiGroup*);

/*! \brief set text on label

    Note that the layout for the label is not updated when setting a new text.
    So, if the new text is too long, part of it might be invisible.
    It's just what Qt decides to do with it.

*/
    virtual void        setText(const char*);
    const char*         text() const;

private:

    uiLabelBody*	body_;
    uiLabelBody&	mkbody(uiParent*,const char*);

};

#endif
