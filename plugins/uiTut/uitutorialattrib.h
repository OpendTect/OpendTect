#ifndef uitutorialattrib_h
#define uitutorialattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        R. K. Singh
 Date:          May 2007
 RCS:           $Id: uitutorialattrib.h,v 1.2 2007-06-08 06:20:01 cvsraman Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; }
class uiAttrSel;
class uiGenInput;
class uiSteeringSel;
class uiStepOutSel;


class uiTutorialAttrib : public uiAttrDescEd
{
public:

			uiTutorialAttrib(uiParent*,bool);

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		actionfld_;
    uiGenInput*		factorfld_;
    uiGenInput*		shiftfld_;
    uiGenInput*		smoothstrengthfld_;
    uiGenInput*         smoothdirfld_;
    uiSteeringSel*      steerfld_;
    uiStepOutSel*       stepoutfld_;


    void		actionSel(CallBacker*);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};

#endif
