#ifndef seis2dline_h
#define seis2dline_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		June 2004
 RCS:		$Id: seis2dline.h,v 1.2 2004-06-18 13:58:07 bert Exp $
________________________________________________________________________

-*/
 
#include "ranges.h"
#include "uidobj.h"
class IOPar;
class Executor;
class SeisTrcBuf;
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

    Executor*		lineFetcher(int,SeisTrcBuf&) const;
    				//!< May return null
    Executor*		lineAdder(IOPar*,const SeisTrcBuf&) const;
    				//!< May return null. If OK, call commitAdd
    void		commitAdd(IOPar*);
    				//!< Must be called after successful add
    void		remove(int);

    static const char*	sKeyZRange;

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
    virtual Executor*	getFetcher(const IOPar&,SeisTrcBuf&)	= 0;
    virtual Executor*	getPutter(IOPar&,const SeisTrcBuf&,
				  const IOPar* prev=0)		= 0;

    const BufferString	type;
    virtual bool	isUsable(const IOPar&) const;
    static const char*	sKeyType;
    static const char*	sKeyLineNr;

protected:

			Seis2DLineIOProvider( const char* t )
    			: type(t)				{}
};


ObjectSet<Seis2DLineIOProvider>& S2DLIOPs();
//!< Sort of factory. Add a new type via this function.


#endif
