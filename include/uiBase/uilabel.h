#ifndef uilabel_H
#define uilabel_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          7/9/2000
 RCS:           $Id: uilabel.h,v 1.3 2001-08-23 14:59:17 windev Exp $
________________________________________________________________________

-*/

#include <uiobj.h>
//class QLabel;

class uiLabelBody;

class uiLabel : public uiObject
{
public:

                        uiLabel(uiParent*,const char*,uiObject* buddy=0);


    virtual void        setText(const char*);
    const char*         text();

private:

    uiLabelBody*	body_;
    uiLabelBody&	mkbody(uiParent*,const char*);

};

#endif
