#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Prajjaval Singh
 Date:		March 2021
 RCS:		$Id$
________________________________________________________________________

-*/


#include "bufstringset.h"
#include "earthmodelmod.h"
#include "executor.h"
#include "multiid.h"
#include "od_istream.h"
#include "posinfo2d.h"
#include "ranges.h"
#include "tableascio.h"
#include "uistrings.h"

class BinIDValueSet;
namespace Survey { class Geometry2D; }
namespace EM { class Horizon2DAscIO; class BulkHorizon2DAscIO; }
namespace Table { class FormatDesc; }


namespace EM
{
mExpClass(EarthModel) BulkHorizon2DScanner : public Executor
{ mODTextTranslationClass(BulkHorizon2DScanner);
public:

			BulkHorizon2DScanner(const BufferStringSet& fnms,
					 Table::FormatDesc& fd);
			~BulkHorizon2DScanner();

    virtual uiString	uiMessage() const;
    virtual od_int64	totalNr() const;
    virtual od_int64	nrDone() const;
    virtual uiString	uiNrDoneText() const;

    ObjectSet<BinIDValueSet>	getVals()		{ return data_; }
    void		getHorizonName(BufferStringSet&) const;
    void		getLineNames(TypeSet<BufferStringSet>&) const;

    bool		reInitAscIO(const char*);

    static const char*	defaultUserInfoFile();
    void		launchBrowser(const char* fnm=0) const;
    void		report(IOPar&) const;
    bool		hasGaps();

protected:
    virtual int			nextStep();

    void			init();

    mutable int			totalnr_    = 0;
    mutable int			nrdone_     = 0;
    EM::BulkHorizon2DAscIO*	ascio_	    = nullptr;
    int				fileidx_;

    Table::FormatDesc&		fd_;

    const Survey::Geometry2D*	curlinegeom_;

    BufferStringSet		hornmset_;
    BufferStringSet		invalidnms_;
    BufferStringSet		filenames_;
    TypeSet<BufferStringSet>	validnms_;
    BufferString		prevhornm_  = BufferString::empty();

    BinIDValueSet*		bvalset_    = nullptr;
    ObjectSet<BinIDValueSet>	data_;

    uiString			msg_	    =  uiStrings::sScanning();

    TypeSet<float>		startzvals_;
    TypeSet<float>		endzvals_;
    TypeSet<TypeSet<Interval<float>>>	valrangesset_;
    int				horidx_     = -1;
};
}
