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
class uiScaler;
class uiSeisSel;
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
    uiGenInput*		remnullfld;
    uiGenInput*		lnmfld;
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
    void		initFlds(CallBacker*);
    bool		acceptOK(CallBacker*);

    static SeisIOSimple::Data&	data2d();
    static SeisIOSimple::Data&	data3d();
    static SeisIOSimple::Data&	dataps();
    SeisIOSimple::Data&	data()
			{ return  geom_ == Seis::Line ? data2d()
			       : (geom_ == Seis::Vol  ? data3d()
				       		      : dataps()); }

    bool		is2D() const	{ return geom_ == Seis::Line; }
    bool		isPS() const	{ return geom_ == Seis::VolPS; }

private:

    void		mkIsAscFld();
    uiSeparator*	mkDataManipFlds();

};


#endif
