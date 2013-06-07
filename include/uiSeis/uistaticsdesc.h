#ifndef uistaticsdesc_h
#define uistaticsdesc_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          May 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseissel.h"
#include "veldesc.h"

class uiGenInput;
class uiSeisSel;
class uiLabeledComboBox;

/*!Group that allows the user to edit StaticsDesc information. */

mClass uiStaticsDesc : public uiGroup
{
public:

    				uiStaticsDesc(uiParent*,const StaticsDesc* s=0);

    bool			get(StaticsDesc&,bool displayerrors) const;
    void			set(const StaticsDesc&);
    bool			updateAndCommit(IOObj&,bool displayerrors);

protected:

    void			updateFlds(CallBacker*);

    uiIOObjSel*			horfld_;
    uiGenInput*			useconstantvelfld_;
    uiGenInput*			constantvelfld_;
    uiLabeledComboBox*		horattribfld_;
};


#endif
