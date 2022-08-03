#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman
 Date:		May 2017
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
class uiConvertGeographicPos;

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

    virtual bool	initFields(const CoordSystem*);

protected:

    uiListBox*		projselfld_;
    uiLineEdit*		searchfld_;
    uiLabeledComboBox*	filtersel_;

    uiConvertGeographicPos*	convdlg_	= nullptr;

    bool			orthogonal_	= true;
    PtrMan<CRSInfoList>		crsinfolist_;
    int				curselidx_	= -1;
    TypeSet<int>		dispidxs_;
				// Indexes of ids_/names_ displayed in ListBox.

    bool		acceptOK();
    void		createGUI();
    void		fetchList();
    void		fillList();
    void		setCurrent();
    void		selChgCB(CallBacker*);
    void		convCB(CallBacker*);
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


mExpClass(uiCRS) uiConvertGeographicPos : public uiDialog
{ mODTextTranslationClass(uiConvertGeographicPos);

public:
			uiConvertGeographicPos(uiParent*,
					ConstRefMan<CoordSystem>,
					const Coord& initialpos);

    void		setCoordSystem(ConstRefMan<CoordSystem>);

private:

    ConstRefMan<CoordSystem>	coordsystem_;

    uiGenInput*		ismanfld_;
    uiGroup*		mangrp_;
    uiGroup*		filegrp_;
    uiGenInput*		dirfld_;
    uiGenInput*		xfld_;
    uiGenInput*		yfld_;
    uiCheckBox*		towgs84fld_;
    uiCheckBox*		fromwgs84fld_;
    uiLatLongInp*	latlngfld_;
    uiFileInput*	inpfilefld_;
    uiFileInput*	outfilefld_;

    void		finalizeCB(CallBacker*);
    void		selChg(CallBacker*);
    void		applyCB(CallBacker*);
    void		convPos();
    void		convFile();
};

} // namespace Coords
