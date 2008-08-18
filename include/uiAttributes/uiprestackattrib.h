#ifndef uiprestackattrib_h
#define uiprestackattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        B.Bril & H.Huck
 Date:          Jan 2008
 RCS:           $Id: uiprestackattrib.h,v 1.6 2008-08-18 08:54:52 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

class CtxtIOObj;
namespace Attrib { class Desc; };

class uiLabel;
class uiGenInput;
class uiSeisSel;


/*! \brief PreStack Attribute ui */

class uiPreStackAttrib : public uiAttrDescEd
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
