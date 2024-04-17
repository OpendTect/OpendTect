#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "executor.h"
#include "bufstringset.h"
#include "ranges.h"

class BinIDValueSet;
namespace Survey { class Geometry2D; }
namespace EM { class Horizon2DAscIO; }
namespace Table { class FormatDesc; }
namespace ZDomain { class Info; }

mExpClass(uiEMAttrib) Horizon2DScanner : public Executor
{ mODTextTranslationClass(Horizon2DScanner);
public:
			mDeprecated("Use Contructor with ZDomain::Info")
			Horizon2DScanner(const BufferStringSet& fnms,
					 Table::FormatDesc& fd);
			~Horizon2DScanner();

    uiString		uiMessage() const override;
    od_int64		totalNr() const override;
    od_int64		nrDone() const override;
    uiString		uiNrDoneText() const override;

    bool		getLineNames(BufferStringSet&) const;
    BinIDValueSet*	getVals()			{ return bvalset_; }

    bool		reInitAscIO(const char*);

    static const char*	defaultUserInfoFile();
    void		launchBrowser(const char* fnm=0) const;
    void		report(StringPairSet&) const;
    mDeprecated("Use with a StringPairSet")
    void		report(IOPar&) const;
    bool		hasGaps();

protected:
    int			nextStep() override;

    void		init();

    uiString		msg_;
    mutable int		totalnr_;
    EM::Horizon2DAscIO*	ascio_;
    BufferStringSet	filenames_;
    int			fileidx_;

    BufferString	curline_;
    bool		isgeom_;
    TypeSet<Interval<float> > valranges_;
    Table::FormatDesc&	fd_;

    const Survey::Geometry2D* curlinegeom_;

    BufferStringSet	linenames_;
    BufferStringSet	validnms_;
    BufferStringSet	invalidnms_;
    BinIDValueSet*	bvalset_;

protected:
    bool		istracenr_;

public:
			    Horizon2DScanner(const BufferStringSet& fnms,
				   Table::FormatDesc& fd,const ZDomain::Info&);
};
