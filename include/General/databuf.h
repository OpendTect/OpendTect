#ifndef databuf_H
#define databuf_H

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		1-9-95
 Contents:	A Data trace is an XFunction with iteration
 RCS:		$Id: databuf.h,v 1.1.1.1 1999-09-03 10:11:41 dgb Exp $
________________________________________________________________________

*/

#include <general.h>


class DataBuffer
{
public:
			DataBuffer(int n,int byts=4,bool init=NO);
			~DataBuffer();
    DataBuffer&  	operator=(const DataBuffer&);

    int			state() const		{ return data ? Ok : Fail; }
    int			size() const		{ return nelem; }
    void		reSize(int);
    void		reByte(int);
    int			bytesPerSample() const	{ return bytes; }
    void		clear();

    unsigned char*	data;

protected:

    int			nelem;
    int			bytes;

};


/*$-*/
#endif
