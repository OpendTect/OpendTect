#ifndef seis2dline_h
#define seis2dline_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		June 2004
 RCS:		$Id: seis2dline.h,v 1.1 2004-06-17 14:56:51 bert Exp $
________________________________________________________________________

-*/
 
#include "ranges.h"
#include "uidobj.h"
class SeisTrcBuf;
class IOPar;

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

    SeisTrcBuf*		getData(int) const;
    void		add(IOPar*,const SeisTrcBuf&);

    void		remove(int);

    static const char*	sKeyZRange;

protected:

    BufferString	fname_;
    ObjectSet<IOPar>	pars_;

    void		init(const char*);
    void		readFile();
    void		writeFile() const;

};


/*!\brief Provides read/write to/from 2D seismic lines.
	  Only interesting if you want to add your own 2D data I/O. */

class Seis2DLineIOProvider
{
public:

    virtual SeisTrcBuf*	getData(const IOPar&)			= 0;
    virtual bool	putData(IOPar&,const SeisTrcBuf&,
	    			const IOPar* prev=0)		= 0;

    BufferString	type;
    BufferString	errmsg;

    virtual bool	isUsable(const IOPar&) const;

    static const char*	sKeyType;
};


ObjectSet<Seis2DLineIOProvider>& S2DLIOP();
//!< Sort of factory. Add a new type via this function.


#endif
