#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		25-10-1996
________________________________________________________________________

-*/

#include "seiscommon.h"

#include "bufstring.h"
#include "datachar.h"
#include "datadistribution.h"
#include "samplingdata.h"
#include "seistype.h"
#include "survgeom.h"
#include "survsubsel.h"

class BinnedValueSet;
class BufferStringSet;
class SeisIOObjInfo;
class TrcKeyZSampling;
namespace Survey { class FullSubSel; }
namespace ZDomain { class Def; }

namespace Seis
{

class Provider;

mExpClass(Seis) ObjectSummary
{
public:

    mUseType( Pos,	GeomID );

			ObjectSummary(const DBKey&,GeomID geomid=mUdfGeomID);
			ObjectSummary(const IOObj&,GeomID geomid=mUdfGeomID);
			ObjectSummary(const ObjectSummary&);
			~ObjectSummary();

    ObjectSummary&	operator =(const ObjectSummary&);

    inline bool		isOK() const	{ return !bad_; }
    inline bool		is2D() const	{ return Seis::is2D(geomtype_); }
    inline bool		isPS() const	{ return Seis::isPS(geomtype_); }

			// return valid stuff if isOK()
    const IOObj*	ioObj() const;
    DBKey		key() const;
    const char*		name() const;

    inline DataType	dataType() const	{ return datatype_; }
    inline GeomType	geomType() const	{ return geomtype_; }
    const StepInterval<float>& zRange() const	{ return zsamp_; }
    inline od_int64	mainFileModifTime() const { return modiftm_; }
    const BufferStringSet& compNames() const	{ return compnms_; }
    inline const DataCharacteristics& dataChar() const
						{ return datachar_;}
    bool		hasSameFormatAs(const BinDataDesc&) const;

    const SeisIOObjInfo& ioObjInfo() const	{ return ioobjinfo_; }

protected:

    const SeisIOObjInfo& ioobjinfo_;
    GeomID		geomid_;

    DataCharacteristics datachar_;
    ZSampling		zsamp_;
    DataType		datatype_;
    GeomType		geomtype_;
    BufferStringSet	compnms_;

    //Cached
    bool		bad_;
    od_int64		modiftm_;
    int			nrsamppertrc_;
    int			nrbytespersamp_;
    int			nrdatabytespespercomptrc_;
    int			nrdatabytespertrc_;
    int			nrbytestrcheader_;
    int			nrbytespertrc_;

private:

    void		init(GeomID);
    void		refreshCache(const Seis::Provider&);
    friend class RawTrcsSequence;

};

}; //namespace Seis


/*!\brief Info on IOObj for seismics */

mExpClass(Seis) SeisIOObjInfo
{ mODTextTranslationClass(SeisIOObjInfo)
public:

    mUseType( Pos,	GeomID );
    mUseType( Seis,	GeomType );

			SeisIOObjInfo(const IOObj*);
			SeisIOObjInfo(const IOObj&);
			SeisIOObjInfo(const DBKey&);
			SeisIOObjInfo(const char* ioobjnm,GeomType);
			SeisIOObjInfo(const SeisIOObjInfo&);
    virtual		~SeisIOObjInfo();
    SeisIOObjInfo&	operator =(const SeisIOObjInfo&);

    inline bool		isOK() const	{ return !bad_; }
    inline bool		is2D() const	{ return Seis::is2D(geomtype_); }
    inline bool		isPS() const	{ return Seis::isPS(geomtype_); }

    GeomType		geomType() const	{ return geomtype_; }
    const IOObj*	ioObj() const		{ return ioobj_; }
    bool		isTime() const;
    bool		isDepth() const;
    const ZDomain::Def&	zDomainDef() const;
    BufferString	iconID() const;

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
    od_int64		getFileModifTime() const;
    Survey::FullSubSel* getSurvSubSel() const;
    Survey::GeomSubSel* getGeomSubSel(GeomID) const;
    bool		getRanges(TrcKeyZSampling&) const;
    bool		isFullyRegular() const;
    bool		getDataChar(DataCharacteristics&) const;
    bool		getBPS(int&,int icomp=0) const;
			    //!< max bytes per sample, component -1 => add all
    bool		havePars() const;
    bool		getPars(IOPar&) const;
    bool		getDisplayPars( IOPar& iop ) const
			{ return getPars(iop); }
    void		saveDisplayPars(const IOPar&);

    void		getUserInfo(uiPhraseSet&) const;
    bool		haveStats() const;
    bool		getStats(IOPar&) const;
    RefMan<FloatDistrib> getDataDistribution() const;
			//this may take some time, use uiUserShowWait or alike

    int			nrComponents(GeomID geomid=mUdfGeomID) const;
    void		getComponentNames(BufferStringSet&,
					  GeomID geomid=mUdfGeomID) const;

    mStruct(Seis) Opts2D
    {
				Opts2D()
				    : bvs_(0), steerpol_(2)	{}
	const BinnedValueSet*	bvs_;
	BufferString		zdomky_;	//!< default=empty=only SI()'s
				//!< Will be matched as GlobExpr
	int			steerpol_;	//!< 0=none, 1=only, 2=both
				//!< Casts into Seis::SteerPol
    };

    // 2D only
    void		getGeomIDs(GeomIDSet&) const;
    void		getLineNames( BufferStringSet& b,
				      Opts2D o2d=Opts2D() ) const
				{ getNms(b,o2d); }
    bool		getRanges(const GeomID geomid,
				  StepInterval<int>& trcrg,
				  StepInterval<float>& zrg) const;

    static void		initDefault(const char* type=0);
			//!< Only does something if there is not yet a default
    static DBKey	getDefault(const char* type=0);
    static void		setDefault(const DBKey&,const char* type=0);

    static bool		hasData(GeomID);
    static void		getDataSetNamesForLine(GeomID,BufferStringSet&,
						Opts2D o2d=Opts2D());
    static void		getDataSetNamesForLine(const char* lnm,BufferStringSet&,
						Opts2D o2d=Opts2D() );
    static void		getLinesWithData(BufferStringSet& lnms,GeomIDSet& gids);
    static bool		isCompatibleType(const char* omftypestr1,
					 const char* omftypestr2);

    static const char*	sKeyPartialScan()   { return "Partial Scan"; }
    static const char*	sKeyFullScan()	    { return "Full Scan"; }

protected:

    GeomType		geomtype_;
    bool		bad_;
    IOObj*		ioobj_;

    void		setType();

    void		getNms(BufferStringSet&,const Opts2D&) const;
    int			getComponentInfo(GeomID,BufferStringSet*) const;
    bool		haveAux(const char* ext) const;
    bool		getAux(const char* ext,const char* ftyp,IOPar&) const;
    void		getCommonUserInfo(uiPhraseSet&) const;
    void		getPostStackUserInfo(uiPhraseSet&) const;
    void		getPreStackUserInfo(uiPhraseSet&) const;

};
