#ifndef seistrctr_h
#define seistrctr_h

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		10-5-1995
 RCS:		$Id: seistrctr.h,v 1.3 2000-03-22 13:41:41 bert Exp $
________________________________________________________________________

Translators for seismic traces.

@$*/

#include <transl.h>
#include <ctxtioobj.h>
#include <seiskeys.h>
#include <storlayout.h>
class Conn;
class SeisTrc;
class SeisTrcInfo;
class BinIDSelector;
class SeisPacketInfo;

#define mXCoordByte	"X-coord byte"
#define mYCoordByte	"Y-coord byte"
#define mInlineByte	"In-line byte"
#define mCrosslineByte	"Cross-line byte"


class SeisTrcTranslator : public Translator
{			  isTranslatorGroup(SeisTrc)

    friend class	TrcStorage;

public:
			SeisTrcTranslator( const char* nm=0,
				    const ClassDef* cd=&StreamConn::classdef )
			: Translator(nm,cd)
			, conn(0)
			, errmsg(0)
			, trustcoords(NO)		{}

			// Init functions are mandatory
			// Conn object must be always available
    virtual bool	initRead( SeisPacketInfo&, Conn& c )
			{ conn = 0; if ( !c.forRead() ) return false;
			  conn = &c; return true; }
    virtual bool	initWrite( const SeisPacketInfo&, Conn& c )
			{ conn = 0; if ( !c.forWrite() ) return false;
			  conn = &c; return true; }
    virtual bool	read(SeisTrcInfo&)		{ return NO; }
    virtual bool	read(SeisTrc&)			{ return NO; }
    virtual bool	skip(unsigned short ns=0)	{ return NO; }
    virtual bool	write(const SeisTrc&)		{ return NO; }
    const char*		errMsg() const			{ return errmsg; }
    void		clearErr()			{ errmsg = 0; }
    virtual void	prepareRetry()			{}

    SeisKeyData&	keyData()			{ return keys; }
    inline int		step() const
			{ return keys.sg.step > 1 ? keys.sg.step : 1; }
    const StorageLayout& storageLayout() const		{ return slo; }
    bool		validCoords() const		{ return trustcoords; }

    static int		selector(const char*);
    static IOObjContext	ioContext();

    virtual void	usePar(const IOPar*);
    virtual int		dataBytes() const		{ return 4; }

protected:

    Conn*		conn;
    SeisKeyData		keys;
    StorageLayout	slo;
    bool		trustcoords;
    const char*		errmsg;

    unsigned char*	trcData(const SeisTrc&) const;

    static const UserIDSet* getICParSpec();

private:

    static UserIDSet	icparspec;

};


#endif
