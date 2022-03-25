#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman
 Date:		May 2017
________________________________________________________________________

-*/

#include "uitoolsmod.h"
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

mExpClass(uiTools) uiProjectionBasedSystem : public uiCoordSystem
{ mODTextTranslationClass(uiProjectionBasedSystem);
public:
    mDefaultFactoryInstantiation1Param( uiCoordSystem,
				uiProjectionBasedSystem, uiParent*,
				ProjectionBasedSystem::sFactoryKeyword(),
				ProjectionBasedSystem::sFactoryDisplayName() );

			uiProjectionBasedSystem(uiParent*);
			~uiProjectionBasedSystem();

    virtual bool	initFields(const CoordSystem*);

protected:

    uiListBox*		projselfld_;
    uiLineEdit*		searchfld_;

    uiConvertGeographicPos*	convdlg_;

    TypeSet<AuthorityCode>	ids_;
    BufferStringSet		names_;
    BufferStringSet		defstrs_;
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
    void		infoCB(CallBacker*);

};


mExpClass(uiTools) uiConvertGeographicPos : public uiDialog
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
