#ifndef seis2dline_h
#define seis2dline_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		June 2004
 RCS:		$Id: seis2dline.h,v 1.13 2004-09-13 07:52:15 bert Exp $
________________________________________________________________________

-*/
 
#include "ranges.h"
#include "uidobj.h"
#include "seistrctr.h"
class IOPar;
class Executor;
class SeisTrcBuf;
class SeisSelData;
class Seis2DLineIOProvider;


/*!\brief interface for object that provides a current 2D line key */

class Seis2DLineKeyProvider
{
public:

    virtual		~Seis2DLineKeyProvider()	{}
    virtual BufferString lineKey() const		= 0;

};


/*!\brief interface for object that writes 2D seismic data */

class Seis2DLinePutter
{
public:
    virtual		~Seis2DLinePutter()	{}

    virtual bool	put(const SeisTrc&)	= 0;
    //!< Return null on success, err msg on failure
    virtual const char* errMsg() const		= 0;
    //!< Only when put returns false
    virtual int	nrWritten() const		= 0;
};


/*!\brief Set of 2D lines comparable with 3D seismic cube */

class Seis2DLineSet : public UserIDObject
{
public:
			Seis2DLineSet( const char* fnm )
			    	: UserIDObject("")	{ init( fnm ); }
			Seis2DLineSet( const Seis2DLineSet& lg )
			    	: UserIDObject(lg.name()) { init(lg.fname_); }
    Seis2DLineSet&	operator=(const Seis2DLineSet&);
    virtual		~Seis2DLineSet();

    const char*		type() const;
    int			nrLines() const			{ return pars_.size(); }
    const IOPar&	getInfo( int idx ) const	{ return *pars_[idx]; }
    bool		isEmpty(int) const;
    const char*		lineName(int) const;	//!< returns pars_[idx]->name()
    const char*		attribute(int) const;
    BufferString	lineKey( int idx ) const
    			{ return lineKey( lineName(idx), attribute(idx) ); }
    int			indexOf(const char* linekey) const;

    Executor*		lineFetcher(int,SeisTrcBuf&,
	    			    const SeisSelData* sd=0) const;
    				//!< May return null
    Seis2DLinePutter*	lineReplacer(int) const;
    				//!< May return null.
    Seis2DLinePutter*	lineAdder(IOPar*) const;
    				//!< May return null. When finished: commitAdd
    				//!< will return replacer if linekey exists
    void		commitAdd(IOPar*);
    				//!< Must be called after successful add
    void		remove(int);
    				//!< Also removes from disk


    bool		getTxtInfo(int,BufferString& uinfo,
	    			   BufferString& stdinfo) const;
    bool		getRanges(int,StepInterval<int>& trcrg,
	    			  StepInterval<float>& zrg) const;

    static const char*	sKeyAttrib;
    static const char*	sKeyDefAttrib;

    static const char*	lineName(const IOPar&);
    static const char*	attribute(const IOPar&);
    static BufferString	lineKey(const IOPar&);
    static BufferString	lineKey(const char* lnm,const char* attrnm);
			//!< "line_name|attribute"
    static BufferString	lineNamefromKey(const char*);
    static BufferString	attrNamefromKey(const char*);
    static void		setLineKey(IOPar&,const char*);

protected:

    Seis2DLineIOProvider* liop_;
    BufferString	fname_;
    ObjectSet<IOPar>	pars_;

    void		init(const char*);
    void		readFile(BufferString* typ=0);
    void		writeFile() const;


};


/*!\brief Provides read/write to/from 2D seismic lines.
	  Only interesting if you want to add your own 2D data I/O. */

class Seis2DLineIOProvider
{
public:

    virtual		~Seis2DLineIOProvider()			{}

    virtual bool	isUsable(const IOPar&) const		{ return true; }

    virtual bool	isEmpty(const IOPar&) const		= 0;
    virtual Executor*	getFetcher(const IOPar&,SeisTrcBuf&,
	    			   const SeisSelData* sd=0)	= 0;
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

class TwoDSeisTrcTranslator : public SeisTrcTranslator
{			isTranslator(TwoD,SeisTrc) public:
			TwoDSeisTrcTranslator( const char* s1, const char* s2 )
			: SeisTrcTranslator(s1,s2)      {}

    const char*		defExtension() const		{ return "2ds"; }
    bool		implRemove(const IOObj*) const;
    bool		initRead_(); // supporting getRanges()
    bool		initWrite_(const SeisTrc&)	{ return false; }

};


#endif
