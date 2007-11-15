#ifndef uiveldesc_h
#define uiveldesc_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          November 2007
 RCS:           $Id: uiveldesc.h,v 1.1 2007-11-15 21:14:46 cvskris Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uigroup.h"
#include "veldesc.h"

class uiGenInput;
class uiSeisSel;
class CtxtIOObj;

class uiVelocityDesc : public uiGroup
{
public:
    				uiVelocityDesc(uiParent*,const VelocityDesc&);

    VelocityDesc		get() const;
    void			set(const VelocityDesc&);

protected:

    uiGenInput*			typefld_;
    uiGenInput*			samplefld_;
};


class uiVelocityDescDlg : public uiDialog
{
public:
    			uiVelocityDescDlg(uiParent*);
			~uiVelocityDescDlg();

protected:
   bool			acceptOK(CallBacker*);
   void			volSelChange(CallBacker*);

   uiSeisSel*		volsel_;
   uiVelocityDesc*	veldesc_;
   CtxtIOObj&		ctxt_;
};

#endif
