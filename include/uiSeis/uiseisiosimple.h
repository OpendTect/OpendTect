#ifndef uiseisiosimple_h
#define uiseisiosimple_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2003
-*/

#include "uidialog.h"
#include "samplingdata.h"
#include "seisiosimple.h"
#include "multiid.h"
class CtxtIOObj;
class uiLabel;
class uiScaler;
class uiSeisSel;
class uiCheckBox;
class uiGenInput;
class uiFileInput;
class uiSeparator;
class uiSeisSubSel;


class uiSeisIOSimple : public uiDialog
{
public:

    			uiSeisIOSimple(uiParent*,Seis::GeomType,bool isimp);

protected:

    uiFileInput*	fnmfld;
    uiGenInput*		isascfld;
    uiGenInput*		is2dfld;
    uiGenInput*		havesdfld;
    uiGenInput*		sdfld;
    uiGenInput*		haveposfld;
    uiGenInput*		havenrfld;
    uiGenInput*		isxyfld;
    uiGenInput*		inldeffld;
    uiGenInput*		crldeffld;
    uiGenInput*		nrdeffld;
    uiGenInput*		startposfld;
    uiGenInput*		startnrfld;
    uiGenInput*		stepposfld;
    uiGenInput*		stepnrfld;
    uiGenInput*		offsdeffld;
    uiGenInput*		remnullfld;
    uiGenInput*		lnmfld;
    uiLabel*		pspposlbl;
    uiCheckBox*		haveoffsbut;
    uiCheckBox*		haveazimbut;
    uiScaler*		scalefld;
    uiSeisSel*		seisfld;
    uiSeisSubSel*	subselfld;

    CtxtIOObj&		ctio;
    Seis::GeomType	geom_;
    bool		isimp_;

    void		isascSel(CallBacker*);
    void		inpSeisSel(CallBacker*);
    void		haveposSel(CallBacker*);
    void		havenrSel(CallBacker*);
    void		havesdSel(CallBacker*);
    void		haveoffsSel(CallBacker*);
    void		initFlds(CallBacker*);
    bool		acceptOK(CallBacker*);

    static SeisIOSimple::Data&	data2d();
    static SeisIOSimple::Data&	data3d();
    static SeisIOSimple::Data&	dataps();
    SeisIOSimple::Data&	data()
			{ return  geom_ == Seis::Line ? data2d()
			       : (geom_ == Seis::Vol  ? data3d()
				       		      : dataps()); }

    bool		is2D() const	{ return Seis::is2D(geom_); }
    bool		isPS() const	{ return Seis::isPS(geom_); }

private:

    void		mkIsAscFld();
    uiSeparator*	mkDataManipFlds();

};


#endif
