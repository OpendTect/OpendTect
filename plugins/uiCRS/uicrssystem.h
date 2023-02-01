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
class uiListBox;
class uiLineEdit;
class uiLatLongInp;

namespace Coords
{

class CRSInfoList;

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

    uiListBox*		projselfld_;
    uiLineEdit*		searchfld_;
    uiLabeledComboBox*	filtersel_;

    bool			orthogonal_	= true;
    PtrMan<CRSInfoList>		crsinfolist_;
    int				curselidx_	= -1;
    TypeSet<int>		dispidxs_;
				// Indexes of ids_/names_ displayed in ListBox.

    bool		acceptOK() override;
    void		createGUI();
    void		fetchList();
    void		fillList();
    void		setCurrent();
    void		selChgCB(CallBacker*);
    void		searchCB(CallBacker*);
    void		infoCB(CallBacker*);

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
