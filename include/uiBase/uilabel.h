#ifndef uilabel_H
#define uilabel_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          7/9/2000
 RCS:           $Id: uilabel.h,v 1.2 2001-01-26 09:54:04 arend Exp $
________________________________________________________________________

-*/

#include <uiobj.h>
class QLabel;
template <class T> class i_QObjWrapper;
mTemplTypeDefT(i_QObjWrapper,QLabel,i_QLabel)

class uiLabel : public uiWrapObj<i_QLabel>
{
public:

                        uiLabel(uiObject*,const char*,uiObject* buddy=0);

    virtual bool        isSingleLine() const { return true; }

    virtual void        setText(const char*);
    const char*         text();

protected:

    const QWidget*	qWidget_() const;
};

#endif
