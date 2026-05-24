#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicrsmod.h"

#include "uilatlong2coord.h"
#include "crssystem.h"

class uiCheckBox;
class uiLabeledComboBox;
class uiFileInput;
class uiLineEdit;
class uiLatLongInp;
class uiTableView;

namespace Coords
{

class CRSInfoList;
class CRSInfoTableModel;

mExpClass(uiCRS) uiProjectionBasedSystem : public uiCoordSystem
{ mODTextTranslationClass(uiProjectionBasedSystem);
public:
    mDefaultFactoryInstantiation1Param( uiCoordSystem,
				uiProjectionBasedSystem, uiParent*,
				ProjectionBasedSystem::sFactoryKeyword(),
				ProjectionBasedSystem::sFactoryDisplayName() );

			uiProjectionBasedSystem(uiParent*);
			uiProjectionBasedSystem(uiParent*,bool orthogonal);
			~uiProjectionBasedSystem();

    bool		initFields(const CoordSystem*) override;

protected:

    uiTableView*	projtable_;
    uiLineEdit*		searchfld_;
    uiLabeledComboBox*	filtersel_;
    uiCheckBox*		showmethodfld_		= nullptr;
    uiCheckBox*		showareafld_;
    uiButton*		gotocurrentbut_;

    bool			orthogonal_	= true;
    const CRSInfoList&		crsinfolist_;
    CRSInfoTableModel*		tablemodel_	= nullptr;
    TypeSet<int>		dispidxs_;

    bool		acceptOK() override;
    void		createGUI();
    void		fillList();
    void		selChgCB(CallBacker*);
    void		searchCB(CallBacker*);
    void		showColumnsCB(CallBacker*);
    void		goToCurrentCB(CallBacker*);
};


mExpClass(uiCRS) uiGeodeticCoordSystem : public uiProjectionBasedSystem
{ mODTextTranslationClass(uiGeodeticCoordSystem);
public:
			uiGeodeticCoordSystem(uiParent* p)
			    : uiProjectionBasedSystem(p,false) {}
			~uiGeodeticCoordSystem() {}

    static uiCoordSystem* getCRSGeodeticFld(uiParent*);

};

} // namespace Coords
