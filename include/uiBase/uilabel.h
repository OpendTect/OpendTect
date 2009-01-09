#ifndef uilabel_h
#define uilabel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          7/9/2000
 RCS:           $Id: uilabel.h,v 1.12 2009-01-09 04:26:14 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
#include "draw.h"

class uiGroup;
class uiLabelBody;

mClass uiLabel : public uiObject
{
public:

                        uiLabel(uiParent*,const char*);
                        uiLabel(uiParent*,const char*,uiObject*);
                        uiLabel(uiParent*,const char*,uiGroup*);

/*! \brief set text on label

    Note that the layout for the label is not updated when setting a new text.
    So, if the new text is too long, part of it might be invisible.
    Therefore, reserve enough space for it with setPrefWidthInChar.

*/
    virtual void        setText(const char*);
    const char*         text() const;

/*! 
    setting an alignment only makes sense if you reserve space using
    setPrefWidthInChar();
*/
    void		setAlignment(OD::Alignment);
    OD::Alignment	alignment() const;

private:

    void		init(const char* txt,uiObject* buddy);

    uiLabelBody*	body_;
    uiLabelBody&	mkbody(uiParent*,const char*);

};

#endif
