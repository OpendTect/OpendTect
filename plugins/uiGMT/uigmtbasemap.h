#ifndef uigmtbasemap_h
#define uigmtbasemap_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uidlggroup.h"

class uiCheckBox;
class uiGenInput;
class uiPushButton;
class uiTextEdit;

class uiGMTBaseMapGrp : public uiDlgGroup
{
public:

    			uiGMTBaseMapGrp(uiParent*);

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		reset();

protected:

    uiGenInput*		titlefld_;
    uiGenInput*		xdimfld_;
    uiGenInput*		ydimfld_;
    uiGenInput*		scalefld_;
    uiGenInput*		xrgfld_;
    uiGenInput*		yrgfld_;
    uiPushButton*	resetbut_;
    uiGenInput*		lebelintvfld_;
    uiCheckBox*		gridlinesfld_;
    uiTextEdit*		remarkfld_;

    float		aspectratio_;

    void		resetCB(CallBacker*);
    void		xyrgChg(CallBacker*);
    void		dimChg(CallBacker*);
    void		scaleChg(CallBacker*);
    void		updateFlds(bool);
};

#endif
