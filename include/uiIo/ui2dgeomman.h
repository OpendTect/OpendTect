#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          September 2010
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uiobjfileman.h"

/*!
\brief Manage window for 2D Line geometries
*/

mExpClass(uiIo) ui2DGeomManageDlg : public uiObjFileMan
{ mODTextTranslationClass(ui2DGeomManageDlg);
public:

			ui2DGeomManageDlg(uiParent*);
			~ui2DGeomManageDlg();

protected:

    void		manLineGeom(CallBacker*);
    void		lineRemoveCB(CallBacker*);
    virtual void	ownSelChg();
    virtual bool	gtItemInfo(const IOObj&,uiPhraseSet&) const;
};
