#ifndef uiprestackattrib_h
#define uiprestackattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        B.Bril & H.Huck
 Date:          Jan 2008
 RCS:           $Id: uiprestackattrib.h,v 1.7 2009-01-08 08:50:11 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

class CtxtIOObj;
namespace Attrib { class Desc; };

class uiLabel;
class uiGenInput;
class uiSeisSel;


/*! \brief PreStack Attribute ui */

mClass uiPreStackAttrib : public uiAttrDescEd
{
public:

			uiPreStackAttrib(uiParent*,bool);
			~uiPreStackAttrib();

protected:

    uiSeisSel*		inpfld_;
    uiGenInput*		offsrgfld_;
    uiGenInput*		calctypefld_;
    uiGenInput*		stattypefld_;
    uiGenInput*		lsqtypefld_;
    uiGenInput*		offsaxtypefld_;
    uiGenInput*		valaxtypefld_;
    uiGenInput*		useazimfld_;
    uiLabel*		xlbl_;

    CtxtIOObj&		ctio_;

    bool		setParameters(const Attrib::Desc&);
    bool		getParameters(Attrib::Desc&);

    void		calcTypSel(CallBacker*);

    			mDeclReqAttribUIFns
};

#endif
