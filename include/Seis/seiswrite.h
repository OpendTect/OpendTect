#ifndef seiswrite_h
#define seiswrite_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		27-1-98
 RCS:		$Id: seiswrite.h,v 1.10 2004-07-16 15:35:25 bert Exp $
________________________________________________________________________

-*/


/*!\brief writes to a seismic data store.

Before first usage, the starter() executable will be executed if you don't
do that separately. If you want to do some component and range selections
using the SeisTrcTranslator, you must first run the starter yourself.

*/

#include <seisstor.h>
class BinIDRange;


class SeisTrcWriter : public SeisStoreAccess
{
public:

			SeisTrcWriter(const IOObj*);
			~SeisTrcWriter();

    bool		prepareWork(const SeisTrc&);
    virtual bool	put(const SeisTrc&);

    void		fillAuxPar(IOPar&) const;
    bool		isMultiComp() const;
    bool		isMultiConn() const;

protected:

    Conn*		crConn(int,bool);
    bool		startWrite(Conn*,const SeisTrc&);
    bool		ensureRightConn(const SeisTrc&,bool);

    bool		prepared;
    BinIDRange&		binids;
    int			nrtrcs;
    int			nrwritten;

    void		startWork();

};


#endif
