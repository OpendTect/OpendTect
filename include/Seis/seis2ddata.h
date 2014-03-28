#ifndef seis2ddata_h
#define seis2ddata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		Dec 2012
 RCS:		$Id$
________________________________________________________________________

-*/
 
#include "seismod.h"
#include "namedobj.h"
#include "objectset.h"
#include "od_iosfwd.h"

class IOObj;
class Executor;
class SeisTrcBuf;
class CubeSampling;
class BinIDValueSet;
class BufferStringSet;
class SeisTrcWriter;
class Seis2DLinePutter;
class Seis2DLineIOProvider;
namespace PosInfo	{ class LineSet2DData; class Line2DData; }
namespace Seis		{ class SelData; }

static const char* sKeyNoOfLines mUnusedVar = "Number of Lines";

/*!\brief Set of 2D lines comparable with 3D seismic cube */


mExpClass(Seis) Seis2DDataSet : public NamedObject
{
    friend class SeisTrcWriter;

public:
			    Seis2DDataSet( const Seis2DDataSet& lg )
			    	: NamedObject(lg.name()) 
				, datatype_(lg.dataType())  { init(lg.fname_); }
			    Seis2DDataSet(const IOObj&);
    virtual		    ~Seis2DDataSet();
    Seis2DDataSet&	    operator=(const Seis2DDataSet&);
    void		    setReadOnly( bool yn=true )	{ readonly_ = yn; }

    const char*		    fileName() const		{ return fname_; }
    const char*		    type() const;
    int			    nrLines() const		{ return pars_.size(); }
    const IOPar&	    getInfo( int idx ) const	{ return *pars_[idx]; }
    bool		    isEmpty(int) const;
    const char*		    lineName(int) const;
    Pos::GeomID		    geomID(int) const;
    Pos::GeomID		    geomID(const char* filename) const;
    int			    indexOf(const char* linename) const;
    int			    indexOf(Pos::GeomID) const;
    const char*		    zDomainKey(int) const;
    const char*		    dataType() const		{   return datatype_; }
    void		    getLineNames(BufferStringSet&) const;
    
    Executor*		    lineFetcher(int,SeisTrcBuf&,int nrtrcsperstep=10,
	    				const Seis::SelData* sd=0) const;
    				//!< May return null
    Seis2DLinePutter*	    linePutter(IOPar*);
    				//!< May return null.
    				//!< will return replacer if geomid exists
    bool		    addLineFrom(Seis2DDataSet&,const char* lnm,
				    const char* datatype=0);

    bool		    getTxtInfo(int,BufferString& uinfo,
	    				BufferString& stdinfo) const;
    bool		    getRanges(int,StepInterval<int>& trcrg,
	    				StepInterval<float>& zrg) const;
    bool		    haveMatch(int,const BinIDValueSet&) const;

    bool		    renameFiles(const char*);
    bool		    remove(Pos::GeomID);
    				//!< Also removes from disk

    static void		    getDataSetsOnLine(const char* lnm,
					      BufferStringSet& ds);
    static void		    getDataSetsOnLine(const Pos::GeomID geomid,
					      BufferStringSet& ds);

protected:

    Seis2DLineIOProvider*   liop_;
    BufferString	    fname_;
    ObjectSet<IOPar>	    pars_;
    BufferString	    datatype_;    
    bool		    readonly_;

    void		    init(const char*);
    void		    readDir();

private:

    bool		    getGeometry(int,PosInfo::Line2DData&) const;
};


#endif
