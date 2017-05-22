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
class uiFileInput;
class uiListBox;
class uiLineEdit;
class uiLatLongInp;

namespace Coords
{

class uiConvertGeographicPos;

mExpClass(uiCRS) uiProjectionBasedSystem : public uiPositionSystem
{ mODTextTranslationClass(uiProjectionBasedSystem);
public:
    mDefaultFactoryInstantiation1Param( uiPositionSystem,
				uiProjectionBasedSystem, uiParent*,
				ProjectionBasedSystem::sFactoryKeyword(),
				ProjectionBasedSystem::sFactoryDisplayName() );

			uiProjectionBasedSystem(uiParent*);
			~uiProjectionBasedSystem();

    virtual bool	initFields(const PositionSystem*);

protected:

    uiListBox*		projselfld_;
    uiLineEdit*		searchfld_;

    uiConvertGeographicPos*	convdlg_;

    TypeSet<AuthorityCode>	ids_;
    BufferStringSet		names_;
    int				curselidx_;
    TypeSet<int>		dispidxs_;
				// Indexes of ids_/names_ displayed in ListBox.

    bool		acceptOK();
    void		fetchList();
    void		fillList();
    void		setCurrent();
    void		selChgCB(CallBacker*);
    void		convCB(CallBacker*);
    void		searchCB(CallBacker*);

};


mExpClass(uiCRS) uiConvertGeographicPos : public uiDialog
{ mODTextTranslationClass(uiConvertGeographicPos);

public:
                        uiConvertGeographicPos(uiParent*,
					ConstRefMan<PositionSystem>,
					const Coord& initialpos);

    void		setCoordSystem(ConstRefMan<PositionSystem>);

private:

    ConstRefMan<PositionSystem>	coordsystem_;

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

    void		finaliseCB(CallBacker*);
    void		selChg(CallBacker*);
    void		applyCB(CallBacker*);
    void		convPos();
    void		convFile();
};

} //Namespace
