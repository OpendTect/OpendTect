#ifndef uihorizonattrib_h
#define uihorizonattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: uihorizonattrib.h,v 1.10 2012-08-29 08:18:06 cvskris Exp $
________________________________________________________________________

-*/

#include "uihorizonattribmod.h"
#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiAttrSel;
class uiGenInput;
class uiIOObjSel;
class uiCheckBox;


/*! \brief Horizon attribute description editor */

class uiHorizonAttrib : public uiAttrDescEd
{
public:

			uiHorizonAttrib(uiParent*,bool);
			~uiHorizonAttrib();

protected:

    uiAttrSel*		inpfld_;
    uiIOObjSel*		horfld_;
    uiGenInput*		typefld_;
    uiGenInput*		surfdatafld_;
    uiCheckBox*		isrelbox_;

    BufferStringSet	surfdatanms_;
    int			nrouttypes_;

    virtual bool	setParameters(const Attrib::Desc&);
    virtual bool	setInput(const Attrib::Desc&);
    virtual bool	getParameters(Attrib::Desc&);
    virtual bool	getInput(Attrib::Desc&);

    void		horSel(CallBacker*);
    void		typeSel(CallBacker*);

    			mDeclReqAttribUIFns
};


#endif
