#ifndef uihorizonattrib_h
#define uihorizonattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: uihorizonattrib.h,v 1.3 2006-10-11 06:59:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class CtxtIOObj;
class uiAttrSel;
class uiGenInput;
class uiIOObjSel;


/*! \brief Coherency attribute description editor */

class uiHorizonAttrib : public uiAttrDescEd
{
public:

			uiHorizonAttrib(uiParent*);
			~uiHorizonAttrib();

protected:

    uiAttrSel*		inpfld_;
    uiIOObjSel*		horfld_;
    uiGenInput*		outputfld_;

    CtxtIOObj&		horctio_;

    virtual bool	setParameters(const Attrib::Desc&);
    virtual bool	setInput(const Attrib::Desc&);
    virtual bool	setOutput(const Attrib::Desc&);
    virtual bool	getParameters(Attrib::Desc&);
    virtual bool	getInput(Attrib::Desc&);
    virtual bool	getOutput(Attrib::Desc&);
    virtual void	set2D(bool);

    void		horSel(CallBacker*);

    			mDeclReqAttribUIFns
};


#endif
