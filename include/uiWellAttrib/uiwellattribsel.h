#ifndef uiwellattribsel_h
#define uiwellattribsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          February 2004
 RCS:           $Id: uiwellattribsel.h,v 1.2 2004-03-18 10:13:40 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class AttribDescSet;
class NLAModel;
class MultiID;
class uiAttrSel;
class uiGenInput;

namespace Well { class Data; };

/*! \brief Dialog for marker specifications */

class uiWellAttribSel : public uiDialog
{
public:
				uiWellAttribSel(uiParent*,Well::Data&,
						const AttribDescSet&,
						const NLAModel* mdl=0);
				~uiWellAttribSel();

    int				newLogIdx() const	{ return newlogidx; }

protected:

    Well::Data&			wd;
    const AttribDescSet&	attrset;
    const NLAModel*		nlamodel;

    uiAttrSel*			attribfld;
    uiGenInput*			rangefld;
    uiGenInput*			lognmfld;

    void			selDone(CallBacker*);
    void			setDefaultRange(bool);
    virtual bool		acceptOK(CallBacker*);
    int				getLogIdx(const char*);

    int				newlogidx;
};



#endif
