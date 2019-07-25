#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2014
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uidialog.h"

class uiFileSel;
class uiGenInput;
class uiGeom2DSel;
class uiIOObjSelGrp;
class uiTableImpDataSel;
namespace Survey { class Geometry2D; }
namespace Table { class FormatDesc; }

mExpClass(uiIo) uiImp2DGeom : public uiDialog
{ mODTextTranslationClass(uiImp2DGeom)
public:
				uiImp2DGeom(uiParent*,const char* lnm=0);
				~uiImp2DGeom();

    bool			fillGeom(SurvGeom2D&);

protected:
    bool			acceptOK();
    od_istream*			getStrm() const;
    bool			fillGeom(ObjectSet<SurvGeom2D>&);
    void			singmultCB(CallBacker*);

    uiFileSel*			fnmfld_;
    uiGenInput*			singlemultifld_;
    uiTableImpDataSel*		dataselfld_;
    uiGeom2DSel*		linefld_;

    BufferString		linenm_;
    Table::FormatDesc*		geomfd_;
};


mExpClass(uiIo) uiExp2DGeom : public uiDialog
{ mODTextTranslationClass(uiExp2DGeom)
public:
				uiExp2DGeom(uiParent*,
						const TypeSet<Pos::GeomID>* =0,
						bool ismodal=false);
				~uiExp2DGeom();

protected:

    bool			acceptOK();
    void			createUI();
    void			setList( CallBacker* );

    uiIOObjSelGrp*		geomfld_;
    uiFileSel*			outfld_;
    TypeSet<Pos::GeomID>	geomidset_;
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
{ mODTextTranslationClass(Geom2DImpHandler);
public:

    static Pos::GeomID	getGeomID(const char* nm,bool overwrpreok=false);
    static bool		getGeomIDs(const BufferStringSet& lnms,
				   GeomIDSet& geomids,bool overwrpreok=false);
			//!< Use while importing several lines in one go.

protected:

    static void		setGeomEmpty(Pos::GeomID);
    static Pos::GeomID	createNewGeom(const char*);
    static bool		confirmOverwrite(const char*);
    static bool		confirmOverwrite(const BufferStringSet&);

};
