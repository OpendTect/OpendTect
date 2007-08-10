#ifndef uieventfreqattrib_h
#define uieventfreqattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Jul 2007
 RCS:           $Id: uieventfreqattrib.h,v 1.2 2007-08-10 11:57:19 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;


/*! \brief DeltaResample Attribute description editor */

class uiEventFreqAttrib : public uiAttrDescEd
{
public:

			uiEventFreqAttrib(uiParent*,bool);

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		typfld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};


#endif
