#ifndef uisegydef_h
#define uisegydef_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegydef.h,v 1.3 2008-09-19 14:28:44 cvsbert Exp $
________________________________________________________________________

-*/

#include "seistype.h"
#include "uigroup.h"
#include "uisegyio.h"
#include "segyfiledef.h"
class IOObj;
class uiLabel;
class uiGenInput;
class uiCheckBox;
class uiTabStack;
class uiFileInput;
namespace SEGY { class TrcHeaderDef; class FileSpec; class FilePars; }


/*!\brief base class for specification of SEG-Y file stuff */

class uiSEGYDefGroup : public uiGroup
{
public:
    			uiSEGYDefGroup( uiParent* p, const char* grpnm,
					bool forread )
			    : uiGroup(p,grpnm)
			    , forread_(forread)				{}

    virtual bool	fillPar(IOPar&,bool permissive=false) const	= 0;
    virtual void	usePar(const IOPar&)				= 0;
    virtual void	use(const IOObj*,bool force)			= 0;

    virtual void	getReport( IOPar& iop ) const
			{ fillPar( iop, true ); }

protected:

    bool		forread_;
};


/*!\brief UI for Specification of SEG-Y in- or output file(s) */

class uiSEGYFileSpec : public uiSEGYDefGroup
{
public:
    			uiSEGYFileSpec(uiParent*,bool forread,IOPar* iop=0);

    bool		fillPar(IOPar&,bool permissive=false) const;
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

    bool		fillPar(IOPar&,bool permissive=false) const;
    void		usePar(const IOPar&);
    void		use(const IOObj*,bool force);

    SEGY::FilePars	getPars() const;
    void		setPars(const SEGY::FilePars&);

protected:

    uiGenInput*		nrsamplesfld_;
    uiGenInput*		fmtfld_;
    uiGenInput*		bytesswappedfld_;

};


/*!\brief UI for Specification of SEG-Y fields needed for proper im/export

  The idea is that you know beforehand whether the file is Rev.1 or not.
  If it's Rev. 1, the positioning part can be dumped.
 
 */

class uiSEGYFileOpts : public uiSEGYDefGroup
{
public:

    class Setup
    {
    public:
				Setup( Seis::GeomType gt,
					uiSEGYIO::Operation op=uiSEGYIO::Read,
					bool isr1=false )
				    : geom_(gt)
				    , operation_(op)
				    , isrev1_(isr1)	{}

	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(uiSEGYIO::Operation,operation)
	mDefSetupMemb(bool,isrev1) //!< Note: write is always Rev.1
    };

			uiSEGYFileOpts(uiParent*,const Setup&,const IOPar* i=0);
			~uiSEGYFileOpts();

    const Setup&	setup() const		{ return setup_; }

    bool		fillPar(IOPar&,bool permissive=false) const;
    void		usePar(const IOPar&);
    void		getReport(IOPar&) const;
    void		use(const IOObj*,bool force);

protected:

    Setup		setup_;
    bool		is2d_;
    bool		isps_;
    bool		permissive_;

    uiTabStack*		ts_;
    uiGroup*		posgrp_;
    uiGroup*		orulegrp_;

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
