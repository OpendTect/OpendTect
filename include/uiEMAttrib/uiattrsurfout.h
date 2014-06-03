#ifndef uiattrsurfout_h
#define uiattrsurfout_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2004
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "uiattremout.h"

class NLAModel;
class uiGenInput;
class uiIOObjSel;
class uiPushButton;
class Array2DInterpol;

namespace Attrib { class DescSet; }

/*! \brief
Surface Output Batch dialog.
Used for calculating attributes on surfaces
*/


mExpClass(uiEMAttrib) uiAttrSurfaceOut : public uiAttrEMOut
{ mODTextTranslationClass(uiAttrSurfaceOut);
public:
    			uiAttrSurfaceOut(uiParent*,const Attrib::DescSet&,
					 const NLAModel*,const MultiID&);
			~uiAttrSurfaceOut();

   void			fillGridPar(IOPar&) const;
protected:

    bool		prepareProcessing();
    bool		fillPar();
    void		attribSel(CallBacker*);
    void		objSelCB(CallBacker*);
    void		fillUdfSelCB(CallBacker*);
    void		settingsCB(CallBacker*);

    uiGenInput*		attrnmfld_;
    uiIOObjSel*		objfld_;
    uiGenInput*		filludffld_;
    uiPushButton*	settingsbut_;
    Array2DInterpol*	interpol_;
    BufferString	methodname_;
};

#endif

