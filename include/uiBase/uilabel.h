#ifndef uilabel_H
#define uilabel_H

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          7/9/2000
 RCS:           $Id: uilabel.h,v 1.5 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/

#include <uiobj.h>
//class QLabel;

class uiLabelBody;

class uiLabel : public uiObject
{
public:

                        uiLabel(uiParent*,const char*,uiObject* buddy=0);

/*! \brief set text on label

    Note that the layout for the label is not updated when setting a new text.
    So, if the new text is too long, part of it might be invisible.
    It's just what Qt decides to do with it.

*/
    virtual void        setText(const char*);
    const char*         text();

private:

    uiLabelBody*	body_;
    uiLabelBody&	mkbody(uiParent*,const char*);

};

#endif
