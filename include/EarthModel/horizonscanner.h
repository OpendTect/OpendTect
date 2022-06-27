#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		Feb 2004
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "executor.h"
#include "bufstringset.h"
#include "ranges.h"

class BinIDValueSet;
class ZAxisTransform;
namespace EM { class Horizon3DAscIO; }
namespace Table { class FormatDesc; }
namespace PosInfo { class Detector; }
namespace ZDomain { class Def; }

/*!
\brief Executor to scan horizons.
*/

mExpClass(EarthModel) HorizonScanner : public Executor
{ mODTextTranslationClass(HorizonScanner);
public:

			HorizonScanner(const BufferStringSet& fnms,
					Table::FormatDesc& fd, bool isgeom,
					ZAxisTransform*, bool iszdepth=false);
    mDeprecatedDef	HorizonScanner(const BufferStringSet& fnms,
					Table::FormatDesc& fd, bool isgeom);
			~HorizonScanner();

    uiString		uiMessage() const override;
    od_int64		totalNr() const override;
    od_int64		nrDone() const override;
    uiString		uiNrDoneText() const override;

    bool		reInitAscIO(const char*);
    void		setPosIsXY(bool yn)		{ isxy_ = yn; }
    bool		posIsXY() const			{ return isxy_; }
    bool		analyzeData();

    int			nrPositions() const;
    StepInterval<int>	inlRg() const;
    StepInterval<int>	crlRg() const;
    bool		gapsFound(bool inl) const;

    static const char*	defaultUserInfoFile();
    void		launchBrowser(const char* fnm=0) const;
    void		report(IOPar&) const;

    void			setZAxisTransform(ZAxisTransform*);
    const ZAxisTransform*	getZAxisTransform() const;

    void		setZInDepth();
    void		setZInTime();
    bool		isZInDepth() const;

    const ObjectSet<BinIDValueSet>& getSections()	{ return sections_; }

protected:

    int				nextStep() override;
    void			transformZIfNeeded(const BinID&,float&) const;

    void			init();
    const Interval<float>	getReasonableZRange() const;

    mutable int			totalnr_;
    int				nrdone_;
    PosInfo::Detector&		dtctor_;
    EM::Horizon3DAscIO*		ascio_;
    BufferStringSet		filenames_;
    int				fileidx_;
    BufferStringSet		rejectedlines_;

    bool			firsttime_;
    bool			isgeom_;
    bool			isxy_;
    bool			selxy_;
    bool			doscale_;
    TypeSet<Interval<float> >	valranges_;
    Table::FormatDesc&		fd_;

    BinIDValueSet*		bvalset_;
    ObjectSet<BinIDValueSet>	sections_;

    ZAxisTransform*		transform_	= nullptr;
    const ZDomain::Def*		zdomain_;

    mutable uiString		curmsg_;
};


