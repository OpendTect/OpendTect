#ifndef seiswrite_h
#define seiswrite_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		27-1-98
 RCS:		$Id: seiswrite.h,v 1.29 2009-07-22 16:01:18 cvsbert Exp $
________________________________________________________________________

-*/


/*!\brief writes to a seismic data store.

Before first usage, the prepareWork() will be called if you don't
do that separately. If you want to do some component and range selections
using the SeisTrcTranslator (3D only), you must do that before prepareWork
is done.

*/

#include "seisstor.h"
#include "linekey.h"
class SeisTrc;
class SeisPSWriter;
class Seis2DLinePutter;


mClass SeisTrcWriter : public SeisStoreAccess
{
public:

			SeisTrcWriter(const IOObj*,
				      const LineKeyProvider* r=0);
				//!< Write to real user entries from '.omf' file
				//!< Can be anything: SEGY - CBVS - database
			SeisTrcWriter(const char*,bool is_2d,bool is_ps);
				//!< Write 'loose' CBVS files
				//!< (or pre-stack: directories) only.
			~SeisTrcWriter();
    virtual bool	close();

    bool		prepareWork(const SeisTrc&);
    virtual bool	put(const SeisTrc&);
    int			nrWritten() const		{ return nrwritten; }

    bool		isMultiComp() const;
    bool		isMultiConn() const;

    void		writeBluntly( bool yn=true )	{ makewrready = !yn; }

    Seis2DLinePutter*	linePutter()			{ return putter; }
    const Seis2DLinePutter* linePutter() const		{ return putter; }

    SeisPSWriter*	psWriter()			{ return pswriter; }
    const SeisPSWriter*	psWriter() const		{ return pswriter; }

    			// 2D
    const LineKeyProvider* lineKeyProvider() const	{ return lkp; }
    void		setLineKeyProvider( const LineKeyProvider* l )
							{ lkp = l; }
				//!< If no lineKeyProvider set,
				//!< seldata's linekey will be used
    void		setAttrib( const char* a )	{ attrib = a; }
				//!< if set, overrules attrib in linekey
    IOPar&		lineAuxPars()			{ return lineauxiopar; }
    void 		setDataType( const char* dt ) 	{ datatype = dt; } 

    static const char*	sKeyWriteBluntly();
    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

protected:

    bool		prepared;
    int			nrtrcs;
    int			nrwritten;
    SeisTrc&		worktrc;
    bool		makewrready;

    void		init();
    void		startWork();

    // PS only
    SeisPSWriter*	pswriter;

    // 3D only
    Conn*		crConn(int,bool);
    bool		ensureRightConn(const SeisTrc&,bool);
    bool		start3DWrite(Conn*,const SeisTrc&);

    // 2D only
    BufferString	attrib;
    Seis2DLinePutter*	putter;
    IOPar&		lineauxiopar;
    LineKey		prevlk;
    const LineKeyProvider* lkp;
    bool		next2DLine();
    BufferString	datatype;
    bool		put2D(const SeisTrc&);

};


#endif
