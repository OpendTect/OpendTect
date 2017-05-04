#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman
 Date:		May 2017
________________________________________________________________________

-*/

#include "uicrsmod.h"
#include "uicoordsystem.h"
#include "crssystem.h"

class uiListBox;
class uiComboBox;

namespace Coords
{

mExpClass(uiCRS) uiProjectionBasedSystem : public uiPositionSystem
{ mODTextTranslationClass(uiProjectionBasedSystem);
public:
    mDefaultFactoryInstantiation1Param( uiPositionSystem,
	    			uiProjectionBasedSystem, uiParent*,
				ProjectionBasedSystem::sFactoryKeyword(),
				ProjectionBasedSystem::sFactoryDisplayName() );

			uiProjectionBasedSystem(uiParent*);

    virtual bool	initFields(const PositionSystem*);

protected:

    uiListBox*		projselfld_;
    uiComboBox*		sortselfld_;

    TypeSet<ProjectionID>	ids_;
    BufferStringSet		names_;
    
    bool		acceptOK();
    void		fetchList();
    void		fillList(bool);
    void		setCurrent(ProjectionID);

};

} //Namespace
