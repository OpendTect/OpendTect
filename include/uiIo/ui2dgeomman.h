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
#include "geomid.h"

class uiGenInput;
class uiPushButton;
class uiTable;
namespace Survey { class Geometry2D; }

/*!
\brief General manage window for 2D Line geometries
*/

mExpClass(uiIo) ui2DGeomManageDlg : public uiObjFileMan
{ mODTextTranslationClass(ui2DGeomManageDlg)
public:
			ui2DGeomManageDlg(uiParent*);
			~ui2DGeomManageDlg();

protected:

    void		manLineGeom(CallBacker*);
    void		lineRemoveCB(CallBacker*);
    virtual void	ownSelChg();
    virtual bool	gtItemInfo(const IOObj&,uiPhraseSet&) const;
};



/*!
\brief Manage window for a single 2D Line geometry
*/

mExpClass(uiIo) uiManageLineGeomDlg : public uiDialog
{ mODTextTranslationClass(uiManageLineGeomDlg)
public:
			uiManageLineGeomDlg(uiParent*,Pos::GeomID,
					    bool readonly);
			~uiManageLineGeomDlg();

protected:

    void		impGeomCB(CallBacker*);
    void		setTrcSPNrCB(CallBacker*);
    void		fillTable(const Survey::Geometry2D&);
    bool		acceptOK();

    uiTable*		table_;
    uiGenInput*		rgfld_;

    Pos::GeomID		geomid_;
    bool		readonly_;
};

