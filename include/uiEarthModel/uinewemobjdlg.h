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
class uiColorInput;
namespace EM { class Object; }


/*!\brief dialog for creating an empty new EM::Object, or base class for one
  that puts stuff in it. */

mExpClass(uiEarthModel) uiNewEMObjectDlg : public uiDialog
{ mODTextTranslationClass(uiNewEMObjectDlg);
public:

    mDefineFactory1ParamInClass(uiNewEMObjectDlg,uiParent*,factory)

			uiNewEMObjectDlg(uiParent*,const uiString&);

    RefMan<EM::Object> getEMObject() const		    { return emobj_; }

protected:

    virtual RefMan<EM::Object>    getNewEMObject() const = 0;

    uiGenInput*		nmfld_;
    uiColorInput*	colorselfld_;

    RefMan<EM::Object>	emobj_;

    static uiPhrase	phrAlreadyLoadedAskForRename();
    static uiPhrase	phrInterpretationDataExist(const uiWord&,const char*);

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

    virtual RefMan<EM::Object>    getNewEMObject() const;
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

    virtual RefMan<EM::Object>    getNewEMObject() const;
};


mExpClass(uiEarthModel) uiNewFlt3DDlg : public uiNewEMObjectDlg
{ mODTextTranslationClass(uiNewFlt3DDlg);
public:

    mDefaultFactoryInstantiation1Param( uiNewEMObjectDlg,
					uiNewFlt3DDlg,uiParent*,
					EM::Fault3D::typeStr(),
					uiStrings::sFault(mPlural))

			uiNewFlt3DDlg(uiParent*);

    RefMan<EM::Fault3D>	getFault3D();

protected:

    bool		acceptOK();

    virtual RefMan<EM::Object>    getNewEMObject() const;

};
