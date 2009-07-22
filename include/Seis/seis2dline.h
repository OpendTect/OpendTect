#ifndef seis2dline_h
#define seis2dline_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		June 2004
 RCS:		$Id: seis2dline.h,v 1.44 2009-07-22 16:01:18 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "namedobj.h"

#include "linekey.h"
#include "position.h"
#include "ranges.h"
#include "seistrctr.h"
#include <iosfwd>

class IOPar;
class Executor;
class SeisTrcBuf;
class CubeSampling;
class BinIDValueSet;
class BufferStringSet;
class Seis2DLineIOProvider;
namespace PosInfo	{ class LineSet2DData; class Line2DData; }
namespace Seis		{ class SelData; }

/*!\brief interface for object that writes 2D seismic data */

mClass Seis2DLinePutter
{
public:
    virtual		~Seis2DLinePutter()	{}

    virtual bool	put(const SeisTrc&)	= 0;
    //!< Return fase on success, err msg on failure
    virtual bool	close()			= 0;
    //!< Return null on success, err msg on failure
    virtual const char* errMsg() const		= 0;
    //!< Only when put or close returns false
    virtual int	nrWritten() const		= 0;
    
};


/*!\brief Set of 2D lines comparable with 3D seismic cube */

mClass Seis2DLineSet : public NamedObject
{
public:
			Seis2DLineSet( const char* fnm )
			    	: NamedObject("")	{ init( fnm ); }
			Seis2DLineSet( const Seis2DLineSet& lg )
			    	: NamedObject(lg.name()) { init(lg.fname_); }
			Seis2DLineSet(const IOObj&);
    Seis2DLineSet&	operator=(const Seis2DLineSet&);
    virtual		~Seis2DLineSet();
    void		setReadOnly( bool yn=true )	{ readonly_ = yn; }

    const char*		type() const;
    int			nrLines() const			{ return pars_.size(); }
    const IOPar&	getInfo( int idx ) const	{ return *pars_[idx]; }
    bool		isEmpty(int) const;
    const char*		lineName(int) const;	//!< returns pars_[idx]->name()
    const char*		attribute(int) const;
    const char*		datatype(int) const;
    LineKey		lineKey( int idx ) const
    			{ return LineKey( lineName(idx), attribute(idx) ); }
    int			indexOf(const char* linekey) const;
    int			indexOfFirstOccurrence(const char* linenm) const;
    void		getAvailableAttributes(BufferStringSet&,
	    				       const char* datatyp=0,
					       bool allowcnstabsent = false,
	    				       bool incl=true) const;
    void		getLineNamesWithAttrib(BufferStringSet&,const char*);

    bool		getGeometry(PosInfo::LineSet2DData&) const;
    bool		getGeometry(int,PosInfo::Line2DData&) const;
    Executor*		geometryDumper(std::ostream&,bool inc_nr,
	    				float z_val=mUdf(float),
	    				const char* linekey=0) const;

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

    bool		rename(const char* lk,const char* newlk);
    				//!< Fails if new line key exists
    				//!< or if LineSet is currently being written
    bool		renameLine(const char* oldlnm,const char* newlnm);
    				//!< Fails if new line name already exists
    				//!< or if LineSet is currently being written
    bool		remove(const char* lk);
    				//!< Also removes from disk
    				//!< Fails if LineSet is currently being written

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

    			// 'PreSet' line sets are not for 'normal' usage
    static void		addPreSetLS(const char*,const char*);
    void		preparePreSet(IOPar& iop,const char* reallskey) const;
    static void		installPreSet(const IOPar&,const char* reallskey,
				      const char* worklskey);

    void		getFrom(std::istream&,BufferString*);
    void		putTo(std::ostream&) const;

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


};


/*!\brief Provides read/write to/from 2D seismic lines.
	  Only interesting if you want to add your own 2D data I/O. */

mClass Seis2DLineIOProvider
{
public:

    virtual		~Seis2DLineIOProvider()			{}

    virtual bool	isUsable(const IOPar&) const		{ return true; }

    virtual bool	isEmpty(const IOPar&) const		= 0;
    virtual bool	getGeometry(const IOPar&,
				    PosInfo::Line2DData&) const	= 0;
    virtual Executor*	getFetcher(const IOPar&,SeisTrcBuf&,int,
	    			   const Seis::SelData* sd=0)	= 0;
    virtual Seis2DLinePutter* getReplacer(const IOPar&)	= 0;
    virtual Seis2DLinePutter* getAdder(IOPar&,const IOPar* prev,
	    				const char* lgrpnm)	= 0;

    virtual bool	getTxtInfo(const IOPar&,BufferString&,
	    			   BufferString&) const		{ return false;}
    virtual bool	getRanges(const IOPar&,StepInterval<int>&,
	    			   StepInterval<float>&) const	{ return false;}

    static const char*	sKeyLineNr;

    virtual void	removeImpl(const IOPar&) const		= 0;

    const char*		type() const			{ return type_.buf(); }

protected:

			Seis2DLineIOProvider( const char* t )
    			: type_(t)				{}

    const BufferString	type_;
};


ObjectSet<Seis2DLineIOProvider>& S2DLIOPs();
//!< Sort of factory. Add a new type via this function.


//------
//! Translator mechanism is only used for selection etc.

mClass TwoDSeisTrcTranslator : public SeisTrcTranslator
{			isTranslator(TwoD,SeisTrc) public:
			TwoDSeisTrcTranslator( const char* s1, const char* s2 )
			: SeisTrcTranslator(s1,s2)      {}

    const char*		defExtension() const		{ return "2ds"; }
    bool		implRemove(const IOObj*) const;
    bool		initRead_();		//!< supporting getRanges()
    bool		initWrite_(const SeisTrc&)	{ return false; }
    bool		isReadDefault() const		{ return true; }

};


#endif
