#ifndef seiscbvs_h
#define seiscbvs_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		April 2001
 RCS:		$Id: seiscbvs.h,v 1.24 2004-07-23 11:53:00 bert Exp $
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


class CBVSSeisTrcTranslator : public SeisTrcTranslator
{			isTranslator(CBVS,SeisTrc)
public:

			CBVSSeisTrcTranslator(const char*,const char*);
			~CBVSSeisTrcTranslator();

    bool		readInfo(SeisTrcInfo&);
    bool		read(SeisTrc&);
    bool		skip(int nrtrcs=1);

    bool		supportsGoTo() const		{ return true; }
    bool		goTo(const BinID&);
    virtual int		bytesOverheadPerTrace() const	{ return 52; }

    const IOPar*	parSpec(Conn::State) const;
    virtual void	usePar(const IOPar&);

    const CBVSReadMgr*	readMgr() const			{ return rdmgr; }
    BinID2Coord		getTransform() const;

    virtual bool	implRemove(const IOObj*) const;
    virtual bool	implRename(const IOObj*,const char*,
	    			   const CallBack* cb=0) const;
    virtual bool	implSetReadOnly(const IOObj*,bool) const;
    virtual const char*	defExtension() const		{ return "cbvs"; }

    bool		minimalHdrs() const		{ return minimalhdrs; }
    void		setMinimalHdrs( bool yn=true )	{ minimalhdrs = yn; }

protected:

    bool		forread;
    bool		headerdone;
    bool		donext;
    int			nrdone;

    // Following variables are inited by commitSelections_
    bool*		compsel;
    bool*		userawdata;
    unsigned char**	blockbufs;
    unsigned char**	targetptrs;
    TraceDataInterpreter** storinterps;
    int			preseldatatype;
    VBrickSpec&		brickspec;

    CBVSReadMgr*	rdmgr;
    CBVSWriteMgr*	wrmgr;
    PosAuxInfo		auxinf;
    bool		minimalhdrs;

    virtual void	cleanUp();
    virtual bool	initRead_();
    virtual bool	initWrite_(const SeisTrc&);
    virtual bool	commitSelections_();
    virtual bool	writeTrc_(const SeisTrc&);
    virtual void	blockDumped(int);
    bool		startWrite();
    bool		toNext();
    bool		getFileName(BufferString&);

private:

    static const IOPar&	datatypeparspec;

    void		destroyVars();

};


#endif
