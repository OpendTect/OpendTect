#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"

#include "datachar.h"
#include "datadistribution.h"
#include "samplingdata.h"
#include "seistype.h"
#include "survgeom.h"
#include "bufstring.h"


class BinIDValueSet;
class BufferStringSet;
class IOObj;
class SeisIOObjInfo;
class SeisTrcTranslator;
class SurveyChanger;
class TrcKeyZSampling;
namespace ZDomain { class Def; }


/*!\brief Summary for a Seismic object */

namespace Seis {

mExpClass(Seis) ObjectSummary
{
public:
			ObjectSummary(const MultiID&);
			ObjectSummary(const DBKey&);
			ObjectSummary(const IOObj&);
			ObjectSummary(const IOObj&,Pos::GeomID);
			ObjectSummary(const ObjectSummary&);
			~ObjectSummary();

    ObjectSummary&	operator =(const ObjectSummary&);

    inline bool		isOK() const	{ return !bad_; }
    inline bool		is2D() const	{ return Seis::is2D(geomtype_); }
    inline bool		isPS() const	{ return Seis::isPS(geomtype_); }

    bool		hasSameFormatAs(const BinDataDesc&) const;
    inline const DataCharacteristics	getDataChar() const
					{ return datachar_; }
    inline GeomType	geomType() const	{ return geomtype_; }
    const StepInterval<float>&	zRange() const		{ return zsamp_; }

    const SeisIOObjInfo&	getFullInformation() const
				{ return ioobjinfo_; }

protected:

    const SeisIOObjInfo&	ioobjinfo_;

    DataCharacteristics datachar_;
    StepInterval<float> zsamp_;
    GeomType		geomtype_;
    BufferStringSet	compnms_;

    //Cached
    bool		bad_;
    int			nrcomp_;
    int			nrsamppertrc_;
    int			nrbytespersamp_;
    int			nrdatabytespespercomptrc_;
    int			nrdatabytespertrc_;
    int			nrbytestrcheader_;
    int			nrbytespertrc_;

private:

    void		init();
    void		init2D(Pos::GeomID);
    void		refreshCache(const SeisTrcTranslator&);

    friend class RawTrcsSequence;

};

} // namespace Seis

/*!\brief Info on IOObj for seismics */

mExpClass(Seis) SeisIOObjInfo
{ mODTextTranslationClass(SeisIOObjInfo)
public:
			SeisIOObjInfo(const IOObj*);
			SeisIOObjInfo(const IOObj&);
			SeisIOObjInfo(const MultiID&);
			SeisIOObjInfo(const DBKey&);
			SeisIOObjInfo(const char* ioobjnm,Seis::GeomType);
			SeisIOObjInfo(const SeisIOObjInfo&);
			~SeisIOObjInfo();

    SeisIOObjInfo&	operator =(const SeisIOObjInfo&);

    inline bool		isOK() const	{ return !bad_; }
    inline bool		is2D() const	{ return geomtype_ > Seis::VolPS; }
    inline bool		isPS() const	{ return geomtype_ == Seis::VolPS
					      || geomtype_ == Seis::LinePS; }

    Seis::GeomType	geomType() const	{ return geomtype_; }
    const IOObj*	ioObj() const		{ return ioobj_; }
    bool		isTime() const;
    bool		isDepth() const;
    const ZDomain::Def&	zDomainDef() const;

    mStruct(Seis) SpaceInfo
    {
			SpaceInfo(int ns=-1,int ntr=-1,int bps=4);
	int		expectedMBs() const;

	int		expectednrsamps;
	int		expectednrtrcs;
	int		maxbytespsamp;
    };

    bool		getDefSpaceInfo(SpaceInfo&) const;
    int			expectedMBs(const SpaceInfo&) const;
    od_int64		getFileSize() const;
    static od_int64	getFileSize(const char* fnm,int& nrfiles);
    bool		getRanges(TrcKeyZSampling&) const;
    bool		isFullyRectAndRegular() const; // Only CBVS
    bool		getDataChar(DataCharacteristics&) const;
    bool		getBPS(int&,int icomp) const;
			//!< max bytes per sample, component -1 => add all

    int			nrComponents(Pos::GeomID geomid=
					    Survey::GM().cUndefGeomID()) const;
    void		getComponentNames(BufferStringSet&,
					  Pos::GeomID geomid=
					  Survey::GM().cUndefGeomID()) const;
    bool		getDisplayPars(IOPar&) const;

    bool		haveAux(const char* ext) const;
    bool		getAux(const char* ext,const char* ftyp,IOPar&) const;
    bool		havePars() const;
    bool		getPars(IOPar&) const;
    bool		haveStats() const;
    bool		getStats(IOPar&) const;
    bool		isAvailableIn(const TrcKeySampling&) const;

    RefMan<FloatDistrib> getDataDistribution() const;
			//this may take some time, use uiUserShowWait or alike

    mStruct(Seis) Opts2D
    {
				Opts2D()
				    : bvs_(0), steerpol_(2)	{}
	const BinIDValueSet*	bvs_;
	BufferString		zdomky_;	//!< default=empty=only SI()'s
				//!< Will be matched as GlobExpr
	int			steerpol_;	//!< 0=none, 1=only, 2=both
				//!< Casts into uiSeisSel::Setup::SteerPol
    };

    // 2D only
    void		getGeomIDs(TypeSet<Pos::GeomID>&) const;
    void		getLineNames( BufferStringSet& b,
				      Opts2D o2d=Opts2D() ) const
				{ getNms(b,o2d); }
    bool		getRanges(const Pos::GeomID geomid,
				  StepInterval<int>& trcrg,
				  StepInterval<float>& zrg) const;

    static void		initDefault(const char* type=0);
			//!< Only does something if there is not yet a default
    static const MultiID& getDefault(const char* type=0);
    static void		setDefault(const MultiID&,const char* type=0);

    static bool		hasData(Pos::GeomID);
    static void		getDataSetNamesForLine( Pos::GeomID geomid,
						BufferStringSet& b,
						Opts2D o2d=Opts2D() );
    static void		getDataSetNamesForLine( const char* nm,
						BufferStringSet& b,
						Opts2D o2d=Opts2D() );
    static void		getCompNames(const MultiID&,BufferStringSet&);
			//!< Function useful in attribute environments
			//!< The 'MultiID' must be IOObj_ID
    static void		getLinesWithData(BufferStringSet& lnms,
					 TypeSet<Pos::GeomID>& gids);
    static bool		isCompatibleType(const char* omftypestr1,
					 const char* omftypestr2);

    void		getUserInfo(uiStringSet&) const;


			mDeprecatedDef SeisIOObjInfo(const char* ioobjnm);
protected:

    Seis::GeomType	geomtype_;
    bool		bad_;
    IOObj*		ioobj_;
    SurveyChanger*	surveychanger_	= nullptr;

    void		setType();

    void		getNms(BufferStringSet&,const Opts2D&) const;
    int			getComponentInfo(Pos::GeomID,BufferStringSet*) const;

    void		getCommonUserInfo(uiStringSet&) const;
    void		getPostStackUserInfo(uiStringSet&) const;
    void		getPreStackUserInfo(uiStringSet&) const;
};
