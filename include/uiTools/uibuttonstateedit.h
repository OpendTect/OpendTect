#ifndef uibuttonstateedit_h
#define uibuttonstateedit_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Feb 2007
 RCS:           $Id: uibuttonstateedit.h,v 1.2 2009-01-08 07:07:01 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class uiGenInput;

/*!Simple field to edit OD::ButtonState. */

mClass uiButtonStateEdit : public uiGroup
{
public:
    		uiButtonStateEdit(uiParent*,const char* label,int initialstate);
    int		getState() const;
protected:
    static BufferString	createName(int);
    uiGenInput*		combobox_;
    TypeSet<int>	states_;
};

#endif
