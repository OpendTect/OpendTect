#ifndef seiswrite_h
#define seiswrite_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		27-1-98
 RCS:		$Id: seiswrite.h,v 1.5 2001-02-13 17:16:09 bert Exp $
________________________________________________________________________

-*/


/*!\brief writes to a seismic data store.

Before first usage, the starter() executable will be executed if you don't
do that separately. If you want to do some component and range selections
using the SeisTrcTranslator, you must first run the starter yourself.

*/

#include <seisstor.h>
class BinIDRange;


class SeisTrcWriter : public SeisStorage
{

    friend class	SeisWriteStarter;

public:

			SeisTrcWriter(const IOObj*);
			~SeisTrcWriter();

    virtual Executor*	starter(const SeisTrc&);
    bool		isMultiComp() const;
    virtual bool	put(const SeisTrc&);

    void		fillAuxPar(IOPar&) const;

protected:

    void		init();
    bool		initWrite(const SeisTrc&);
    bool		openConn();
    bool		handleConn(const SeisTrc&);

    BinIDRange&		binids;
    int			nrwrconn;
    bool		wrinited;

};


#endif
