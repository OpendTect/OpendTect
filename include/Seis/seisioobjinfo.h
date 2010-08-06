#ifndef seisioobjinfo_h
#define seisioobjinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		25-10-1996
 RCS:		$Id: seisioobjinfo.h,v 1.17 2010-08-06 10:44:32 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "samplingdata.h"
#include "seistype.h"
#include "linekey.h"

class IOObj;
class LineKey;
class MultiID;
class CubeSampling;
class BinIDValueSet;
class BufferStringSet;
namespace ZDomain { class Def; }

/*!\brief Info on IOObj for seismics */

mClass SeisIOObjInfo
{
public:

			SeisIOObjInfo(const IOObj*);
			SeisIOObjInfo(const IOObj&);
			SeisIOObjInfo(const MultiID&);
			SeisIOObjInfo(const char* ioobjnm);
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

    mStruct SpaceInfo
    {
			SpaceInfo(int ns=-1,int ntr=-1,int bps=4);
	int		expectednrsamps;
	int		expectednrtrcs;
	int		maxbytespsamp;
    };

    bool		getDefSpaceInfo(SpaceInfo&) const;
    int			expectedMBs(const SpaceInfo&) const;
    bool		getRanges(CubeSampling&) const;
    bool		getBPS(int&,int icomp) const;
    			//!< max bytes per sample, component -1 => add all

    void		getDefKeys(BufferStringSet&,bool add=false) const;
    			//!< For 3D: IOObj ID, for 2D: list of ID|attrnm
    static BufferString	defKey2DispName(const char* defkey,
	    				const char* ioobjnm=0);

    int			nrComponents(LineKey lk=LineKey()) const;
    void		getComponentNames(BufferStringSet&,
	    				LineKey lk=LineKey()) const;

    // 2D only
    void		getLineNames( BufferStringSet& b, bool add=true,
	    				const BinIDValueSet* bvs=0 ) const
				{ getNms(b,add,false,bvs); }
    void		getAttribNames( BufferStringSet& b, bool add=true,
	    				const BinIDValueSet* bvs=0,
	   				const char* datatyp=0,
	   				bool allowcnstabsent=false,
	   				bool incl=true ) const
				{ getNms(b,add,true,bvs,
					 datatyp,allowcnstabsent,incl); }
    void		getAttribNamesForLine( const char* nm,
	    				       BufferStringSet& b,
					       bool add=true,
	   				       const char* datatyp=0,
	   				       bool allowcnstabsent=false,
	   				       bool incl=true ) const
				{ getNmsSubSel(nm,b,add,false,
					       datatyp,allowcnstabsent,incl); }
    void		getLineNamesWithAttrib( const char* nm,
	    				       BufferStringSet& b,
					       bool add=true ) const
				{ getNmsSubSel(nm,b,add,true); }
    bool		getRanges(const LineKey& lk,StepInterval<int>& trcrg,
	    			  StepInterval<float>& zrg) const;

    static void		initDefault(const char* type=0);
    			//!< Only does something if there is not yet a default
    static const MultiID& getDefault(const char* type=0);
    static void		setDefault(const MultiID&,const char* type=0);

    static void		get2DLineInfo(BufferStringSet& linesets,
	    			      TypeSet<MultiID>* setids=0,
				      TypeSet<BufferStringSet>* linenames=0);
    static void		getCompNames(const LineKey&,BufferStringSet&);
    			//!< Function useful in attribute environments
    			//!< The 'LineKey' must be IOObj_ID|attrnm

protected:

    Seis::GeomType	geomtype_;
    bool		bad_;
    IOObj*		ioobj_;

    void		setType();

    void		getNms(BufferStringSet&,bool,bool,
	    			const BinIDValueSet*,
				const char* datatype=0,
				bool allowcnstabsent=false,
				bool incl=true) const;
    void		getNmsSubSel(const char*,BufferStringSet&,
	    				bool,bool,
					const char* datatype=0,
					bool allowcnstabsent=false,
					bool incl=true) const;
    int			getComponentInfo(LineKey,BufferStringSet*) const;

};


#endif
