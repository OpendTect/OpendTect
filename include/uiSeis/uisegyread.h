#ifndef uisegyread_h
#define uisegyread_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegyread.h,v 1.2 2008-09-24 11:21:38 cvsbert Exp $
________________________________________________________________________

-*/

#include "seistype.h"
#include "iopar.h"
class IOObj;
class uiParent;
namespace SEGY { class Scanner; }


/*!\brief 'Server' for SEG-Y Reading */

class uiSEGYRead
{
public:

    enum Purpose	{ Import, SurvSetup, DirectDef };

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
    int			rev_;
    int			nrexamine_;

    mutable int		state_;
    bool		needsetupdlg_;
    SEGY::Scanner*	scanner_;

    void		getBasicOpts();
    void		getFileOpts();
    void		doScan();
    void		doImport();

    int			targetState() const;
};


#endif
