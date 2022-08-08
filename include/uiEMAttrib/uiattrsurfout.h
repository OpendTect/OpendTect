#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2004
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

    bool		prepareProcessing() override;
    bool		fillPar(IOPar&) override;
    void		attribSel(CallBacker*) override;
    void		objSelCB(CallBacker*);
    void		fillUdfSelCB(CallBacker*);
    void		settingsCB(CallBacker*);
    void		getJobName(BufferString&) const override;

    uiGenInput*		attrnmfld_;
    uiIOObjSel*		objfld_;
    uiGenInput*		filludffld_;
    uiPushButton*	settingsbut_;
    Array2DInterpol*	interpol_;
    BufferString	methodname_;
};

