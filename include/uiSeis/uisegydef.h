#ifndef uisegydef_h
#define uisegydef_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegydef.h,v 1.2 2008-09-15 10:10:36 cvsbert Exp $
________________________________________________________________________

-*/

#include "seistype.h"
#include "uigroup.h"
#include "segyfiledef.h"
class IOObj;
class uiLabel;
class uiGenInput;
class uiFileInput;
class uiCheckBox;
namespace SEGY { class TrcHeaderDef; class FileSpec; class FilePars; }


/*!\brief base class for specification of SEG-Y file stuff */

class uiSEGYDefGroup : public uiGroup
{
public:
    			uiSEGYDefGroup( uiParent* p, const char* grpnm,
					bool forread )
			    : uiGroup(p,grpnm)
			    , forread_(forread)		{}

    virtual bool	fillPar(IOPar&) const		= 0;
    virtual void	usePar(const IOPar&)		= 0;
    virtual void	use(const IOObj*,bool force)	= 0;

    virtual void	getReport( IOPar& iop ) const
			{ fillPar( iop ); }

protected:

    bool		forread_;
};


/*!\brief UI for Specification of SEG-Y in- or output file(s) */

class uiSEGYFileSpec : public uiSEGYDefGroup
{
public:
    			uiSEGYFileSpec(uiParent*,bool forread,IOPar* iop=0);

    bool		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
    void		use(const IOObj*,bool force);

    SEGY::FileSpec	getSpec() const;
    void		setSpec(const SEGY::FileSpec&);

protected:

    uiFileInput*	fnmfld_;
    uiGenInput*		multifld_;

    void		setMultiInput(const StepInterval<int>&,int);

};


/*!\brief UI for Specification of SEG-Y information needed to examine */

class uiSEGYFilePars : public uiSEGYDefGroup
{
public:
    			uiSEGYFilePars(uiParent*,bool forread,IOPar* iop=0);

    bool		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
    void		use(const IOObj*,bool force);

    SEGY::FilePars	getPars() const;
    void		setPars(const SEGY::FilePars&);

protected:

    uiGenInput*		nrsamplesfld_;
    uiGenInput*		fmtfld_;
    uiGenInput*		bytesswappedfld_;

};


/*!\brief UI for Specification of SEG-Y fields needed for proper im/export */

class uiSEGYFileOpts : public uiSEGYDefGroup
{
public:

    enum Purpose	{ Read, Scan, Write };

			uiSEGYFileOpts(uiParent*,Purpose,Seis::GeomType,
				  const IOPar* iop=0);
			~uiSEGYFileOpts();

    Purpose		purpose() const		{ return purpose_; }
    Seis::GeomType	geomType() const	{ return geom_; }

    bool		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
    void		getReport(IOPar&) const;
    void		use(const IOObj*,bool force);

protected:

    Purpose		purpose_;
    Seis::GeomType	geom_;
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
    uiLabel*		ensurepsxylbl_;

    uiGenInput*		scalcofld_;
    uiGenInput*		timeshiftfld_;
    uiGenInput*		sampleratefld_;
    uiCheckBox*		forcerev0fld_;

    void		buildUI(const IOPar*);
    uiGroup*		mkPosGrp(const IOPar*);
    uiGroup*		mkORuleGrp(const IOPar*);

    void		mkTrcNrFlds(uiGroup*,const IOPar*,
	    			    const SEGY::TrcHeaderDef&);
    void		mkBinIDFlds(uiGroup*,const IOPar*,
	    			    const SEGY::TrcHeaderDef&);
    void		mkPreStackPosFlds(uiGroup*,const IOPar*,
					  const SEGY::TrcHeaderDef&);
    void		mkCoordFlds(uiGroup*,const IOPar*,
	    			    const SEGY::TrcHeaderDef&);
    void		attachPosFlds(uiGroup*);

    void		positioningChg(CallBacker*);

    int			posType() const;
    bool		haveIC() const;
    bool		haveXY() const;

};


#endif
