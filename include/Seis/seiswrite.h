#ifndef seiswrite_h
#define seiswrite_h

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		27-1-98
 RCS:		$Id: seiswrite.h,v 1.2 2000-01-24 16:35:10 bert Exp $
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
    bool		initWrite();
    bool		openConn();
    bool		handleConn(const SeisTrcInfo&);

    BinIDRange*		binids;
    float		starttime;
    int			nrsamps;
    unsigned short	dt;
    int			nrwr;
    int			nrwrconn;

};


#endif
