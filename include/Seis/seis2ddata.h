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
#include "position.h"

#include <iosfwd>

class IOPar;
class IOObj;
class Executor;
class SeisTrcBuf;
class CubeSampling;
class SeisTrcWriter;
class Seis2DLinePutter;
class Seis2DLineIOProvider;
class OD_2DLineGeometryFrom2DLinesTransf;
template <class T> class StepInterval;
namespace PosInfo	{ class LineSet2DData; class Line2DData; }
namespace Seis		{ class SelData; }

static const char* sKeyNoOfLines mUnusedVar = "Number of Lines";

/*!\brief Set of 2D lines comparable with 3D seismic cube */

mExpClass(Seis) Seis2DDataSet : public NamedObject
{
    friend class SeisTrcWriter;
    friend class OD_2DLineGeometryFrom2DLinesTransf;

public:
			Seis2DDataSet( const char* fnm )
			    	: NamedObject("")	{ init( fnm ); }
			Seis2DDataSet( const Seis2DDataSet& lg )
			    	: NamedObject(lg.name()) { init(lg.fname_); }
			Seis2DDataSet(const IOObj&);
    Seis2DDataSet&	operator=(const Seis2DDataSet&);
    virtual		~Seis2DDataSet();
    void		setReadOnly( bool yn=true )	{ readonly_ = yn; }

    const char*		fileName() const		{ return fname_; }
    const char*		type() const;
    int			nrLines() const			{ return pars_.size(); }
    const IOPar&	getInfo( int idx ) const	{ return *pars_[idx]; }
    bool		isEmpty(int) const;
    const char*		lineName(int) const;
    TraceID::GeomID	geomID(int) const;
    int			indexOf(const char* linename) const;
    int			indexOf(TraceID::GeomID geomid) const;

    
    Executor*		lineFetcher(int,SeisTrcBuf&,int nrtrcsperstep=10,
	    			    const Seis::SelData* sd=0) const;
    				//!< May return null
    Seis2DLinePutter*	linePutter(IOPar*);
    				//!< May return null.
    				//!< will return replacer if linekey exists
    
    bool		renameFiles(const char*);

    bool		getTxtInfo(int,BufferString& uinfo,
	    			   BufferString& stdinfo) const;
    bool		getRanges(int,StepInterval<int>& trcrg,
	    			  StepInterval<float>& zrg) const;
    const char*		getCubeSampling(CubeSampling&,int linenr=-1) const;
    			//!< returns err msg, or null when OK
   

    void		getFrom(std::istream&,BufferString*);
    void		putTo(std::ostream&) const;

    bool		remove(TraceID::GeomID geomid);
    				//!< Also removes from disk

protected:

    Seis2DLineIOProvider* liop_;
    BufferString	fname_;
    ObjectSet<IOPar>	pars_;
    bool		readonly_;

    void		init(const char*);
    void		readFile(bool mklock=false,BufferString* typ=0);
    void		writeFile() const;

private:
    bool		getGeometry(int,PosInfo::Line2DData&) const;
    Executor*		geometryDumper(std::ostream&,bool inc_nr,
	    				float z_val=mUdf(float),
	    				const char* linekey=0) const;

};


#endif
