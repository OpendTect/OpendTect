#ifndef uisegyread_h
#define uisegyread_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegyread.h,v 1.6 2008-10-02 14:40:06 cvsbert Exp $
________________________________________________________________________

-*/

#include "seistype.h"
#include "iopar.h"
class IOObj;
class uiParent;
class CtxtIOObj;
namespace SEGY { class Scanner; }


/*!\brief 'Server' for SEG-Y Reading */

class uiSEGYRead : public CallBacker
{
public:

    enum Purpose	{ Import, SurvSetup, DirectDef };
    enum RevType	{ Rev0, WeakRev1, Rev1 };

    struct Setup
    {
			Setup( Purpose pp=Import )
			    : purpose_(pp)	{ getDefaultTypes(geoms_);}

	mDefSetupMemb(Purpose,	purpose)
	TypeSet<Seis::GeomType>	geoms_;	//!< Default all

	bool		forScan() const		{ return purpose_ != Import; }
	static void	getDefaultTypes(TypeSet<Seis::GeomType>&);

    };

			uiSEGYRead(uiParent*,const Setup&);
			~uiSEGYRead();

    void		use(const IOObj*,bool force);
    void		usePar(const IOPar&);

    bool		go();

    Seis::GeomType	geomType() const	{ return geom_; }
    int			revision() const	{ return rev_; }
    void		fillPar(IOPar&) const;
    SEGY::Scanner*	getScanner()		//!< Becomes yours
			{ SEGY::Scanner* s = scanner_; scanner_ = 0; return s; }

protected:

    Setup		setup_;
    uiParent*		parent_;
    Seis::GeomType	geom_;
    IOPar		pars_;
    RevType		rev_;
    int			nrexamine_;

    mutable int		state_;
    bool		specincomplete_;
    SEGY::Scanner*	scanner_;

    void		getBasicOpts();
    void		setupScan();
    void		doScan();
    void		doImport();

    void		readReq(CallBacker*);
    void		writeReq(CallBacker*);
    void		preScanReq(CallBacker*);

    void		setGeomType(const IOObj&);
    CtxtIOObj*		getCtio(bool) const;
};


#endif
