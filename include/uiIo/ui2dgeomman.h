#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          September 2010
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uiobjfileman.h"

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
    void		ownSelChg();
    void		mkFileInfo();
};



/*!
\brief Manage window for a single 2D Line geometry
*/

mExpClass(uiIo) uiManageLineGeomDlg : public uiDialog
{ mODTextTranslationClass(uiManageLineGeomDlg)
public:
			uiManageLineGeomDlg(uiParent*,
				const TypeSet<Pos::GeomID>&,bool readonly);
			~uiManageLineGeomDlg();

protected:

    void		impGeomCB(CallBacker*);
    void		expGeomCB(CallBacker*);
    void		lineSel(CallBacker*);
    void		setTrcSPNrCB(CallBacker*);
    void		fillTable(const Survey::Geometry2D&);
    bool		acceptOK(CallBacker*);

    uiTable*		table_;
    uiGenInput*		rgfld_;
    uiGenInput*			linefld_;
    TypeSet<Pos::GeomID>	geomidset_;

    Pos::GeomID		geomid_;	// not used
    bool		readonly_;
};



/*!
\brief This class has a set of static functions handling 2D geometries during
seismic import routines that eventually use a SeisTrcWriter.
While importing 2D seismic data you just need to call:

	Geom2DImpHandler::getGeomID(linename);

to get the GeomID of the line being imported. Geom2DImpHandler will take care
of creating new lines in the database or overwriting them.
*/

mExpClass(uiIo) Geom2DImpHandler
{ mODTextTranslationClass(Geom2DImpHandler)
public:

    static Pos::GeomID	getGeomID(const char* nm,bool overwrpreok=false);
    static bool		getGeomIDs(const BufferStringSet& lnms,
				   TypeSet<Pos::GeomID>& geomids,
				   bool overwrpreok=false);
			//!< Use while importing several lines in one go.

protected:

    static void		setGeomEmpty(Pos::GeomID);
    static Pos::GeomID	createNewGeom(const char*);
    static bool		confirmOverwrite(const char*);
    static bool		confirmOverwrite(const BufferStringSet&);

};

