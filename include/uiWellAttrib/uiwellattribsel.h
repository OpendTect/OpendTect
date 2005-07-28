#ifndef uiwellattribsel_h
#define uiwellattribsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          February 2004
 RCS:           $Id: uiwellattribsel.h,v 1.3 2005-07-28 10:53:49 cvshelene Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class NLAModel;
class MultiID;
class uiAttrSel;
class uiGenInput;

namespace Attrib { class DescSet; }
namespace Well { class Data; };

/*! \brief Dialog for marker specifications */

class uiWellAttribSel : public uiDialog
{
public:
				uiWellAttribSel(uiParent*,Well::Data&,
						const Attrib::DescSet&,
						const NLAModel* mdl=0);
				~uiWellAttribSel();

    int				newLogIdx() const	{ return newlogidx; }

protected:

    Well::Data&			wd;
    const Attrib::DescSet&	attrset;
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
