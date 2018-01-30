#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay
 Date:          Jan 2017
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"

#include "emfault3d.h"
#include "emfaultstickset.h"
#include "emhorizon3d.h"
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

    RefMan<EM::EMObject>	emobj_;
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

    RefMan<EM::FaultStickSet>	getFSS();

protected:

    bool		acceptOK();

    virtual RefMan<EM::EMObject>    getNewEMObject() const;
};


mExpClass(uiEarthModel) uiNewHorizon3DDlg : public uiNewEMObjectDlg
{ mODTextTranslationClass(uiNewHorizon3DDlg);
public:
    mDefaultFactoryInstantiation1Param(
				uiNewEMObjectDlg,
				uiNewHorizon3DDlg,uiParent*,
				EM::Horizon3D::typeStr(),
				uiStrings::sHorizon(mPlural))

			uiNewHorizon3DDlg(uiParent*);

    RefMan<EM::Horizon3D>	getHorizon3D();

protected:

    bool		acceptOK();

    virtual RefMan<EM::EMObject>    getNewEMObject() const;
};


mExpClass(uiEarthModel) uiNewFlt3DDlg : public uiNewEMObjectDlg
{ mODTextTranslationClass(uiNewFlt3DDlg);
public:
    mDefaultFactoryInstantiation1Param( uiNewEMObjectDlg,
					uiNewFlt3DDlg,uiParent*,
					EM::Fault3D::typeStr(),
					uiStrings::sFault(mPlural))

			uiNewFlt3DDlg(uiParent*);

protected:

    bool		acceptOK();

    virtual RefMan<EM::EMObject>    getNewEMObject() const;
};

