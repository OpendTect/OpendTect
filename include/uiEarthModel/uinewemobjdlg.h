#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay
 Date:          Jan 2017
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"

#include "emfaultstickset.h"
#include "uidialog.h"


class uiGenInput;



/*!\brief dialog for creating an empty new EM::EMObject, or base class for one
  that puts stuff in it. */
class uiColorInput;
namespace EM{ class EMObject; }

mExpClass(uiEarthModel) uiNewEMObjectDlg : public uiDialog
{ mODTextTranslationClass(uiNewEMObjectDlg);
public:
    
    mDefineFactory1ParamInClass(uiNewEMObjectDlg,uiParent*,factory)

			uiNewEMObjectDlg(uiParent*,const uiString&);

    RefMan<EM::EMObject> getEMObject() const		    { return emobj_; }

protected:

    virtual RefMan<EM::EMObject>    getNewEMObject() const = 0;

    uiGenInput*		nmfld_;
    uiColorInput*	colorselfld_;
    EM::EMObject*	emobj_;
};


mExpClass(uiEarthModel) uiNewFSSDlg : public uiNewEMObjectDlg
{ mODTextTranslationClass(uiNewFSSDlg);
public:
    mDefaultFactoryInstantiation1Param(
				uiNewEMObjectDlg,
				uiNewFSSDlg,uiParent*,
				EM::FaultStickSet::typeStr(),
				uiStrings::sFaultStickSet(mPlural))

			uiNewFSSDlg(uiParent*);

protected:

    bool		acceptOK();
    
    virtual RefMan<EM::EMObject>    getNewEMObject() const;
};
