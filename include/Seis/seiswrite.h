#ifndef seiswrite_h
#define seiswrite_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		27-1-98
 RCS:		$Id: seiswrite.h,v 1.11 2004-09-03 15:13:14 bert Exp $
________________________________________________________________________

-*/


/*!\brief writes to a seismic data store.

Before first usage, the starter() executable will be executed if you don't
do that separately. If you want to do some component and range selections
using the SeisTrcTranslator, you must first run the starter yourself.

*/

#include "seisstor.h"
class BinIDRange;
class Seis2DLinePutter;
class Seis2DLineKeyProvider;


class SeisTrcWriter : public SeisStoreAccess
{
public:

			SeisTrcWriter(const IOObj*,
				      const Seis2DLineKeyProvider* r=0);
			~SeisTrcWriter();
    void		close();

    bool		prepareWork(const SeisTrc&);
    virtual bool	put(const SeisTrc&);

    void		fillAuxPar(IOPar&) const;
    bool		isMultiComp() const;
    bool		isMultiConn() const;

    			// 2D
    const Seis2DLineKeyProvider* lineKeyProvider() const { return lkp; }
    void		setLineKeyProvider( const Seis2DLineKeyProvider* l )
							 { lkp = l; }
    void		setAttrib( const char* a )	 { attrib = a; }
    			//!< if set, overrules attrib in linekey

protected:

    bool		prepared;
    BinIDRange&		binids;
    int			nrtrcs;
    int			nrwritten;

    void		startWork();

    // 3D only
    Conn*		crConn(int,bool);
    bool		ensureRightConn(const SeisTrc&,bool);
    bool		start3DWrite(Conn*,const SeisTrc&);

    // 2D only
    BufferString	attrib;
    Seis2DLinePutter*	putter;
    IOPar*		lineiopar;
    BufferString	prevlk;
    const Seis2DLineKeyProvider* lkp;
    bool		next2DLine();
    bool		put2D(const SeisTrc&);

};


#endif
