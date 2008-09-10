#ifndef uisegydef_h
#define uisegydef_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegydef.h,v 1.1 2008-09-10 09:05:37 cvsbert Exp $
________________________________________________________________________

-*/

#include "seistype.h"
#include "uigroup.h"
class uiLabel;
class uiGenInput;
class uiIOObjSel;
class uiFileInput;
class uiCheckBox;
class SegyTraceheaderDef;


class uiSEGYDef : public uiGroup
{
public:

    enum Purpose	{ Read, Scan, Write };

			uiSEGYDef(uiParent*,Purpose,Seis::GeomType,
				  const IOPar* iop=0);
			~uiSEGYDef();

    Purpose		purpose() const		{ return purpose_; }
    Seis::GeomType	geomType() const	{ return geom_; }

    bool		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
    void		getReport(IOPar&) const;

protected:

    Purpose		purpose_;
    Seis::GeomType	geom_;

    bool		isrd_;
    bool		is2d_;
    bool		isps_;

    uiGenInput*		positioningfld_;
    uiGenInput*		inlbytefld_;
    uiGenInput*		inlbyteszfld_;
    uiGenInput*		crlbytefld_;
    uiGenInput*		crlbyteszfld_;
    uiGenInput*		trnrbytefld_;
    uiGenInput*		trnrbyteszfld_;
    uiGenInput*		offsbytefld_;
    uiGenInput*		offsbyteszfld_;
    uiGenInput*		azimbytefld_;
    uiGenInput*		azimbyteszfld_;
    uiGenInput*		xcoordbytefld_;
    uiGenInput*		ycoordbytefld_;
    uiGenInput*		regoffsfld_;
    uiLabel*		ensurepsxylbl;

    uiGenInput*		scalcofld_;
    uiGenInput*		timeshiftfld_;
    uiGenInput*		sampleratefld_;
    uiGenInput*		nrsamplesfld_;
    uiCheckBox*		forcerev0fld_;

    uiGenInput*		fmtfld_;

    void		buildUI(const IOPar*);
    uiGroup*		mkPosGrp(const IOPar*);
    uiGroup*		mkORuleGrp(const IOPar*);

    void		mkTrcNrFlds(uiGroup*,const IOPar*,
	    			    const SegyTraceheaderDef&);
    void		mkBinIDFlds(uiGroup*,const IOPar*,
	    			    const SegyTraceheaderDef&);
    void		mkPreStackPosFlds(uiGroup*,const IOPar*,
					  const SegyTraceheaderDef&);
    void		mkCoordFlds(uiGroup*,const IOPar*,
	    			    const SegyTraceheaderDef&);
    void		attachPosFlds(uiGroup*);

    void		positioningChg(CallBacker*);

    int			posType() const;
    bool		haveIC() const;
    bool		haveXY() const;
    void		setNumbFmtFromPar(const IOPar&);

};


#endif
