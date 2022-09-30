#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uigroup.h"
#include "factory.h"
#include "uizaxistransform.h"

class uiIOObjSel;
class uiGenInput;
class uiComboBox;
class uiT2DConvSelGroup;


/*! \brief single-line object for selecting T to depth conversion. */

mExpClass(uiIo) uiT2DConvSel : public uiZAxisTransformSel
{ mODTextTranslationClass(uiT2DConvSel);
public:

    mExpClass(uiIo) Setup
    {
    public:
			Setup(uiIOObjSel* tiedto,bool opt=true);
			~Setup();

	mDefSetupMemb(BufferString,fldtext)
	mDefSetupMemb(bool,optional)
	mDefSetupMemb(uiIOObjSel*,tiedto)
	mDefSetupMemb(bool,ist2d)
    };

			uiT2DConvSel(uiParent*,const Setup&);
			~uiT2DConvSel();

protected:

    Setup				setup_;

    void				inpSel(CallBacker*);
};
