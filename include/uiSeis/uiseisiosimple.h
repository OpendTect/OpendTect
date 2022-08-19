#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "samplingdata.h"
#include "seisiosimple.h"
#include "multiid.h"
#include "uicoordsystem.h"

class IOObjContext;
class uiLabel;
class uiScaler;
class uiSeisSel;
class uiCheckBox;
class uiGenInput;
class uiFileInput;
class uiSeparator;
class uiSeisSubSel;
class uiSeis2DLineNameSel;


mExpClass(uiSeis) uiSeisIOSimple : public uiDialog
{ mODTextTranslationClass(uiSeisIOSimple);
public:
			uiSeisIOSimple(uiParent*,Seis::GeomType,bool isimp);
			~uiSeisIOSimple();

protected:

    uiFileInput*	fnmfld_;
    uiGenInput*		isascfld_;
    uiGenInput*		havesdfld_;
    uiGenInput*		sdfld_				= nullptr;
    uiGenInput*		haveposfld_;
    uiGenInput*		havenrfld_			= nullptr;
    uiGenInput*		haverefnrfld_			= nullptr;
    uiGenInput*		isxyfld_			= nullptr;
    uiGenInput*		inldeffld_			= nullptr;
    uiGenInput*		crldeffld_			= nullptr;
    uiGenInput*		nrdeffld_			= nullptr;
    uiGenInput*		startposfld_			= nullptr;
    uiGenInput*		startnrfld_			= nullptr;
    uiGenInput*		stepposfld_			= nullptr;
    uiGenInput*		stepnrfld_			= nullptr;
    uiGenInput*		offsdeffld_			= nullptr;
    uiLabel*		pspposlbl_			= nullptr;
    uiCheckBox*		haveoffsbut_			= nullptr;
    uiCheckBox*		haveazimbut_			= nullptr;
    uiScaler*		scalefld_;
    uiGenInput*		remnullfld_;
    uiGenInput*		multcompfld_;
    uiSeisSel*		seisfld_;
    uiSeisSubSel*	subselfld_			= nullptr;
    uiSeis2DLineNameSel* lnmfld_			= nullptr;
    Coords::uiCoordSystemSel*	coordsysselfld_		= nullptr;

    Seis::GeomType	geom_;
    bool		isimp_;

    void		isascSel(CallBacker*);
    void		inpSeisSel(CallBacker*);
    void		inpFileSel(CallBacker*);
    void		lsSel(CallBacker*);
    void		haveposSel(CallBacker*);
    void		havenrSel(CallBacker*);
    void		havesdSel(CallBacker*);
    void		haveoffsSel(CallBacker*);
    void		initFlds(CallBacker*);
    bool		acceptOK(CallBacker*) override;
    void		positionInFileSelChg(CallBacker*);

    static SeisIOSimple::Data&	data2d();
    static SeisIOSimple::Data&	data3d();
    static SeisIOSimple::Data&	dataps();
    SeisIOSimple::Data&	data()
			{ return geom_ == Seis::Line ? data2d()
			      : (geom_ == Seis::Vol  ? data3d()
						     : dataps()); }

    bool		is2D() const	{ return Seis::is2D(geom_); }
    bool		isPS() const	{ return Seis::isPS(geom_); }

private:

    void		mkIsAscFld();
    uiSeparator*	mkDataManipFlds();
};
