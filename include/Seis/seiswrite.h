#ifndef seiswrite_h
#define seiswrite_h

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		27-1-98
 RCS:		$Id: seiswrite.h,v 1.1.1.1 1999-09-03 10:11:41 dgb Exp $
________________________________________________________________________

A SeisTrcWriter writes to a seismic data store. To be able to use the writer,
you need to execute the starter() executable. Be sure to have set the
SeisPacketInfo before that. You can fill the info by asking a SeisTrc to do
that.

@$*/

#include <seisstor.h>
class BinIDRange;


class SeisTrcWriter : public SeisStorage
{

    friend class	SeisWriteStarter;

public:

			SeisTrcWriter(const IOObj*);
    virtual Executor*	starter();
    virtual bool	put(const SeisTrc&);

    void		setPacketInfo( const SeisPacketInfo& s ) { spi = s; }
    void		fillAuxPar(IOPar&) const;

protected:

    void		init();
    bool		openFirst();
    bool		initWrite();
    bool		nextConn();

    BinIDRange*		binids;
    float		starttime;
    int			nrsamps;
    unsigned short	dt;
    int			ntrcs;

};


#endif
