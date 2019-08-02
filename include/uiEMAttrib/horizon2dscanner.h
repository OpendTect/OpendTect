#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		May 2008
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "executor.h"
#include "bufstringset.h"
#include "posinfo2d.h"
#include "ranges.h"
#include "dbkey.h"

class BinnedValueSet;
namespace Survey { class Geometry2D; }
namespace EM { class Horizon2DAscIO; }
namespace Table { class FormatDesc; }

mExpClass(uiEMAttrib) Horizon2DScanner : public Executor
{ mODTextTranslationClass(Horizon2DScanner);
public:

			Horizon2DScanner(const BufferStringSet& fnms,
					 Table::FormatDesc& fd);

    virtual uiString	message() const;
    virtual od_int64	totalNr() const;
    virtual od_int64	nrDone() const;
    virtual uiString	nrDoneText() const;

    bool		getLineNames(BufferStringSet&) const;
    BinnedValueSet*	getVals()			{ return bvalset_; }

    bool		reInitAscIO(const char*);

    static const char*	defaultUserInfoFile();
    void		launchBrowser(const char* fnm=0) const;
    void		report(IOPar&) const;

protected:
    virtual int		nextStep();

    void		init();

    mutable int		totalnr_;
    EM::Horizon2DAscIO*	ascio_;
    BufferStringSet	filenames_;
    int			fileidx_;

    BufferString	curline_;
    bool		isgeom_;
    TypeSet<Interval<float> > valranges_;
    Table::FormatDesc&	fd_;

    const SurvGeom2D*	curlinegeom_;

    BufferStringSet	linenames_;
    BufferStringSet	validnms_;
    BufferStringSet	invalidnms_;
    BinnedValueSet*	bvalset_;
};
