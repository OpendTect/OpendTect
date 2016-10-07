#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		25-10-1996
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "samplingdata.h"
#include "seistype.h"
#include "survgeom.h"
#include "bufstring.h"


class DataCharacteristics;
class IOObj;
class TrcKeyZSampling;
class BinIDValueSet;
class BufferStringSet;
namespace ZDomain { class Def; }

/*!\brief Info on IOObj for seismics */

mExpClass(Seis) SeisIOObjInfo
{
public:

			SeisIOObjInfo(const IOObj*);
			SeisIOObjInfo(const IOObj&);
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
    bool		getRanges(TrcKeyZSampling&) const;
    bool		isFullyRectAndRegular() const; // Only CBVS
    bool		getDataChar(DataCharacteristics&) const;
    bool		getBPS(int&,int icomp) const;
			//!< max bytes per sample, component -1 => add all
    bool		fillStats(IOPar&) const;

    int			nrComponents(Pos::GeomID geomid=
					    Survey::GM().cUndefGeomID()) const;
    void		getComponentNames(BufferStringSet&,
					  Pos::GeomID geomid=
					  Survey::GM().cUndefGeomID()) const;
    bool		getDisplayPars(IOPar&) const;

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
    static DBKey	getDefault(const char* type=0);
    static void		setDefault(const DBKey&,const char* type=0);

    static bool		hasData(Pos::GeomID);
    static void		getDataSetNamesForLine( Pos::GeomID geomid,
						BufferStringSet& b,
						Opts2D o2d=Opts2D() );
    static void		getDataSetNamesForLine( const char* nm,
						BufferStringSet& b,
						Opts2D o2d=Opts2D() );
    static void		getCompNames(const DBKey&,BufferStringSet&);
			//!< Function useful in attribute environments
			//!< The 'DBKey' must be IOObj_ID
    static void		getLinesWithData(BufferStringSet& lnms,
					 TypeSet<Pos::GeomID>& gids);

protected:

    Seis::GeomType	geomtype_;
    bool		bad_;
    IOObj*		ioobj_;

    void		setType();

    void		getNms(BufferStringSet&,const Opts2D&) const;
    int			getComponentInfo(Pos::GeomID,BufferStringSet*) const;

};
