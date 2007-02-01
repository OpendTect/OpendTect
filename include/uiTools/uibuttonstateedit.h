#ifndef uibuttonstateedit_h
#define uibuttonstateedit_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Feb 2007
 RCS:           $Id: uibuttonstateedit.h,v 1.1 2007-02-01 22:57:45 cvskris Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class uiGenInput;

/*!Simple field to edit OD::ButtonState. */

class uiButtonStateEdit : public uiGroup
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
