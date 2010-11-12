#ifndef seiscbvs_h
#define seiscbvs_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		April 2001
 RCS:		$Id: seiscbvs.h,v 1.44 2010-11-12 15:02:24 cvsbert Exp $
________________________________________________________________________

CBVS-based seimic translator.

-*/

#include "seistrctr.h"
#include "tracedata.h"
#include "cbvsinfo.h"
class CBVSReadMgr;
class CBVSWriteMgr;
class VBrickSpec;
class SeisTrcBuf;


mClass CBVSSeisTrcTranslator : public SeisTrcTranslator
{			isTranslator(CBVS,SeisTrc)
public:

			CBVSSeisTrcTranslator(const char*,const char*);
    static CBVSSeisTrcTranslator* make(const char* fnm,bool forinfoonly,
	    				bool is2d, BufferString* errmsg=0);
			~CBVSSeisTrcTranslator();

    bool		readInfo(SeisTrcInfo&);
    bool		read(SeisTrc&);
    bool		skip(int nrtrcs=1);

    bool		supportsGoTo() const		{ return true; }
    bool		goTo(const BinID&);
    bool		toStart();
    virtual int		bytesOverheadPerTrace() const	{ return 52; }

    virtual void	usePar(const IOPar&);

    const CBVSReadMgr*	readMgr() const			{ return rdmgr; }
    RCol2Coord		getTransform() const;

    virtual bool	implRemove(const IOObj*) const;
    virtual bool	implRename(const IOObj*,const char*,
	    			   const CallBack* cb=0) const;
    virtual bool	implSetReadOnly(const IOObj*,bool) const;
    const char*	defExtension() const	{ return sKeyDefExtension(); }
    static const char*	sKeyDefExtension();
    static const char*	sKeyDataStorage();

    bool		minimalHdrs() const		{ return minimalhdrs; }
    void		setMinimalHdrs( bool yn=true )	{ minimalhdrs = yn; }
    bool		is2D() const			{ return is2d; }
    void		set2D(bool yn=true);
    bool		singleFile() const		{ return single_file; }
    void		setSingleFile( bool yn=true )	{ single_file = yn; }

    void		setCoordPol(bool dowrite,bool intrailer);
    void		setPreselDataType( int dt )	{ preseldatatype = dt; }

    bool		isReadDefault() const		{ return true; }

protected:

    bool		forread;
    bool		headerdone;
    bool		donext;
    int			nrdone;
    int			coordpol;

    // Following variables are inited by commitSelections_
    bool*		compsel;
    unsigned char**	blockbufs;
    TraceDataInterpreter** storinterps;
    int			preseldatatype;
    VBrickSpec&		brickspec;

    CBVSReadMgr*	rdmgr;
    CBVSWriteMgr*	wrmgr;
    PosAuxInfo		auxinf;
    bool		is2d;
    bool		minimalhdrs;
    bool		single_file;

    virtual void	cleanUp();
    virtual bool	initRead_();
    virtual bool	initWrite_(const SeisTrc&);
    virtual bool	commitSelections_();
    virtual bool	writeTrc_(const SeisTrc&);
    virtual void	blockDumped(int);
    bool		startWrite();
    bool		toNext();
    bool		getFileName(BufferString&);
    bool		inactiveSelData() const;
    int			selRes(const BinID&) const;

private:

    static const IOPar&	datatypeparspec;

    void		destroyVars(int);

};


#endif
