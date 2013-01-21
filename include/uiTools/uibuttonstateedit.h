#ifndef uibuttonstateedit_h
#define uibuttonstateedit_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Feb 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"

class uiGenInput;

/*!Simple field to edit OD::ButtonState. */

mExpClass(uiTools) uiButtonStateEdit : public uiGroup
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

