#ifndef uilabel_h
#define uilabel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          7/9/2000
 RCS:           $Id: uilabel.h,v 1.10 2008-05-07 06:06:23 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiobj.h"

class uiGroup;
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
    Therefore, reserve enough space for it with setPrefWidthInChar.

*/
    virtual void        setText(const char*);
    const char*         text() const;

    enum horAlign {
        AlignAuto               = 0x0000,  
        AlignLeft               = 0x0001,
        AlignRight              = 0x0002,
        AlignHCenter            = 0x0004
    };

/*! 
    setting an alignment only makes sense if you reserve space using
    setPrefWidthInChar();
*/
    void		setAlignment( int );
    int			alignment() const;

private:

    void		init(const char* txt,uiObject* buddy);

    uiLabelBody*	body_;
    uiLabelBody&	mkbody(uiParent*,const char*);

};

#endif
