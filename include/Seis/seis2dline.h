#ifndef seis2dline_h
#define seis2dline_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		June 2004
 RCS:		$Id: seis2dline.h,v 1.5 2004-08-25 12:27:06 bert Exp $
________________________________________________________________________

-*/
 
#include "ranges.h"
#include "uidobj.h"
class IOPar;
class Executor;
class SeisTrcBuf;
class SeisSelData;
class Seis2DLineIOProvider;


/*!\brief Set of 2D lines comparable with 3D seismic cube */

class Seis2DLineGroup : public UserIDObject
{
public:
			Seis2DLineGroup( const char* fnm )
			    	: UserIDObject("")	{ init( fnm ); }
			Seis2DLineGroup( const Seis2DLineGroup& lg )
			    	: UserIDObject(lg.name()) { init(lg.fname_); }
    Seis2DLineGroup&	operator=(const Seis2DLineGroup&);
    virtual		~Seis2DLineGroup();

    int			nrLines() const			{ return pars_.size(); }
    const IOPar&	getInfo( int idx ) const	{ return *pars_[idx]; }
    bool		isEmpty(int) const;
    const char*		lineName(int) const;	//!< returns pars_[idx]->name()
    const char*		attribute(int) const;
    BufferString	lineKey(int) const; //!< "line_name|attribute"

    Executor*		lineFetcher(int,SeisTrcBuf&,
	    			    const SeisSelData* sd=0) const;
    				//!< May return null
    Executor*		lineAdder(IOPar*,const SeisTrcBuf&) const;
    				//!< May return null. If OK, call commitAdd
    void		commitAdd(IOPar*);
    				//!< Must be called after successful add
    void		remove(int); //!< Also removes from disk


    bool		getTxtInfo(int,BufferString& uinfo,
	    			   BufferString& stdinfo) const;
    bool		getRanges(int,StepInterval<int>& trcrg,
	    			  StepInterval<float>& zrg) const;

    static const char*	sKeyAttrib;
    static const char*	sKeyDefAttrib;

protected:

    BufferString	fname_;
    ObjectSet<IOPar>	pars_;

    void		init(const char*);
    void		readFile();
    void		writeFile() const;

    Seis2DLineIOProvider* getLiop(int) const;
    Seis2DLineIOProvider* getLiop(const IOPar&) const;

};


/*!\brief Provides read/write to/from 2D seismic lines.
	  Only interesting if you want to add your own 2D data I/O. */

class Seis2DLineIOProvider
{
public:

    virtual		~Seis2DLineIOProvider()			{}

    virtual bool	isEmpty(const IOPar&) const		= 0;
    virtual Executor*	getFetcher(const IOPar&,SeisTrcBuf&,
	    			   const SeisSelData* sd=0)	= 0;
    virtual Executor*	getPutter(IOPar&,const SeisTrcBuf&,
				  const IOPar* prev=0)		= 0;
    virtual bool	getTxtInfo(const IOPar&,BufferString&,
	    			   BufferString&) const		{ return false;}
    virtual bool	getRanges(const IOPar&,StepInterval<int>&,
	    			   StepInterval<float>&) const	{ return false;}

    virtual bool	isUsable(const IOPar&) const;
    static const char*	sKeyType;
    static const char*	sKeyLineNr;

    virtual void	removeImpl(const IOPar&) const		= 0;

    const BufferString	type;

protected:

			Seis2DLineIOProvider( const char* t )
    			: type(t)				{}
};


ObjectSet<Seis2DLineIOProvider>& S2DLIOPs();
//!< Sort of factory. Add a new type via this function.


#endif
