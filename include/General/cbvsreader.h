#ifndef cbvsreader_h
#define cbvsreader_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format header
 RCS:		$Id: cbvsreader.h,v 1.1 2001-03-19 10:17:57 bert Exp $
________________________________________________________________________

-*/

#include <cbvsinfo.h>
#include <iostream.h>
template <class T> class DataInterpreter;


/*!\brief Reader for CBVS format */

class CBVSReader
{
public:

			CBVSReader(istream*);
    bool		failed() const	{ return errmsg_ ? true : false; }
    const char*		errMsg() const	{ return errmsg_; }
    const CBVSInfo&	info() const	{ return info_; }

    bool		goTo(const BinID&,int icomp=0,int isamp=0);
    bool		moveTo(int icomp,int isamp);
    bool		toNext(int icomp=0,int isamp=0);
    bool		toNextCrl(int,int icomp=0,int isamp=0);

    void		fetch(void*,int nrbytes);
    static const char*	check(istream&);
			//!< Determines whether a file is a CBVS file
			//!< returns an error message or null if OK.

protected:

    istream&		strm_;
    CBVSInfo		info_;
    const char*		errmsg_;

    void		getExplicits(const unsigned char*);
    bool		readComps(int,const DataInterpreter<int>&,
				  const DataInterpreter<float>&);
    bool		readGeom(const DataInterpreter<int>&,
				  const DataInterpreter<double>&);
    bool		readTrailer(const DataInterpreter<int>&);
    bool		readBinIDBounds(const DataInterpreter<int>&);

private:

    const streampos	datastartfo;
    streampos		lastposfo;
    BinID		curbinid;
    int			curinldataidx;

    streampos		readInfo();

};


#endif
