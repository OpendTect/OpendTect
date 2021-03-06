#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2014
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

class uiFileInput;
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

    bool			fillGeom(Survey::Geometry2D&);

protected:
    bool			acceptOK(CallBacker*);
    od_istream*			getStrm() const;
    bool			fillGeom(ObjectSet<Survey::Geometry2D>&);
    void			fileSelCB(CallBacker*);
    void			singmultCB(CallBacker*);

    uiFileInput*		fnmfld_;
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

    bool			acceptOK(CallBacker*);
    void			createUI();
    void			setList( CallBacker* );

    uiIOObjSelGrp*		geomfld_;
    uiFileInput*		outfld_;
    TypeSet<Pos::GeomID>	geomidset_;
};
