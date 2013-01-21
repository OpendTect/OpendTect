#ifndef seis2dline_h
#define seis2dline_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2004
 RCS:		$Id$
________________________________________________________________________

-*/
 
#include "seismod.h"
#include "namedobj.h"
#include "linekey.h"
#include "objectset.h"
#include <iosfwd>

class IOPar;
class IOObj;
class Executor;
class SeisTrcBuf;
class CubeSampling;
class BinIDValueSet;
class BufferStringSet;
class SeisTrcWriter;
class Seis2DLinePutter;
class Seis2DLineIOProvider;
class OD_2DLineGeometryFrom2DLinesTransf;
template <class T> class StepInterval;
namespace PosInfo	{ class LineSet2DData; class Line2DData; }
namespace Seis		{ class SelData; }


/*!\brief Set of 2D lines comparable with 3D seismic cube */

mExpClass(Seis) Seis2DLineSet : public NamedObject
{
    friend class SeisTrcWriter;
    friend class OD_2DLineGeometryFrom2DLinesTransf;

public:
			Seis2DLineSet( const char* fnm )
			    	: NamedObject("")	{ init( fnm ); }
			Seis2DLineSet( const Seis2DLineSet& lg )
			    	: NamedObject(lg.name()) { init(lg.fname_); }
			Seis2DLineSet(const IOObj&);
    Seis2DLineSet&	operator=(const Seis2DLineSet&);
    virtual		~Seis2DLineSet();
    void		setReadOnly( bool yn=true )	{ readonly_ = yn; }

    const char*		fileName() const		{ return fname_; }
    const char*		type() const;
    int			nrLines() const			{ return pars_.size(); }
    const IOPar&	getInfo( int idx ) const	{ return *pars_[idx]; }
    bool		isEmpty(int) const;
    const char*		lineName(int) const;	//!< returns pars_[idx]->name()
    const char*		attribute(int) const;
    const char*		datatype(int) const;
    const char*		zDomainKey(int) const;
    LineKey		lineKey( int idx ) const
    			{ return LineKey( lineName(idx), attribute(idx) ); }
    int			indexOf(const char* linekey) const;
    int			indexOfFirstOccurrence(const char* linenm) const;
    void		getZDomainAttrib(BufferStringSet&,const char* linenm,
	    				  const char* zdomaintype);
    void		getAvailableAttributes(BufferStringSet&,
	    				       const char* datatyp=0,
					       bool allowcnstabsent = false,
	    				       bool incl=true) const;
    void		getLineNamesWithAttrib(BufferStringSet&,
	    					const char* attrnm) const;

    
    Executor*		lineFetcher(int,SeisTrcBuf&,int nrtrcsperstep=10,
	    			    const Seis::SelData* sd=0) const;
    				//!< May return null
    Seis2DLinePutter*	linePutter(IOPar*);
    				//!< May return null.
    				//!< will return replacer if linekey exists
    bool		addLineKeys(Seis2DLineSet&,const char* attrnm,
				    const char* lnm=0,const char* datatype=0);
    				//!< if lnm == null, add attrib to all lines
    				//!< will commit to file

    bool		rename(const char* lsnm);
    				//!< Renames LineSet
    bool		rename(const char* lk,const char* newlk);
    				//!< Fails if new line key exists
    				//!< or if LineSet is currently being written
    bool		renameLine(const char* oldlnm,const char* newlnm);
    				//!< Fails if new line name already exists
    				//!< or if LineSet is currently being written
    bool		remove(const char* lk);
    				//!< Also removes from disk
    				//!< Fails if LineSet is currently being written
    bool		renameFiles(const char*);

    bool		getTxtInfo(int,BufferString& uinfo,
	    			   BufferString& stdinfo) const;
    bool		getRanges(int,StepInterval<int>& trcrg,
	    			  StepInterval<float>& zrg) const;
    const char*		getCubeSampling(CubeSampling&,int linenr=-1) const;
    			//!< returns err msg, or null when OK
    const char*		getCubeSampling(CubeSampling&,const LineKey&) const;
    			//!< returns err msg, or null when OK
    bool		haveMatch(int,const BinIDValueSet&) const;
    			//!< Uses getGeometry

    void		getFrom(std::istream&,BufferString*);
    void		putTo(std::ostream&) const;

    static void		invalidateCache();

protected:

    Seis2DLineIOProvider* liop_;
    BufferString	fname_;
    ObjectSet<IOPar>	pars_;
    bool		readonly_;

    void		init(const char*);
    void		readFile(bool mklock=false,BufferString* typ=0);
    bool		getPre(BufferString*);
    void		writeFile() const;
    void		removeLock() const;


public:

    			// 'PreSet' line sets are not for 'normal' usage
    static void		addPreSetLS(const char*,const char*);
    void		preparePreSet(IOPar& iop,const char* reallskey) const;
    static void		installPreSet(const IOPar&,const char* reallskey,
				      const char* worklskey);

private:
    bool		getGeometry(PosInfo::LineSet2DData&) const;
    bool		getGeometry(int,PosInfo::Line2DData&) const;
    Executor*		geometryDumper(std::ostream&,bool inc_nr,
	    				float z_val=mUdf(float),
	    				const char* linekey=0) const;

};


#endif

