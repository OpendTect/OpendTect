#ifndef cbvswriter_h
#define cbvswriter_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format writer
 RCS:		$Id: cbvswriter.h,v 1.1 2001-03-19 10:17:57 bert Exp $
________________________________________________________________________

-*/

#include <cbvsinfo.h>
#include <iostream.h>
template <class T> class DataInterpreter;


/*!\brief Writer for CBVS format

Works on an ostream that will be deleted on destruction, or when finished.
*/

class CBVSWriter
{
public:

			CBVSWriter(ostream*,const CBVSInfo&,
				   const CBVSInfo::ExplicitData* =0);
			//!< If info.explinfo has a true, the ExplicitData
			//!< is mandatory. The relevant field(s) should then be
			//!< filled before the first put() of any position
			~CBVSWriter();

    bool		failed() const		{ return errmsg_; }
    const char*		errMsg() const		{ return errmsg_; }
    unsigned long	byteThreshold() const	{ return thrbytes_; }		
			//!< The default is unlimited
    void		setByteThreshold( unsigned long n )
						{ thrbytes_ = n; }		

    int			put(void**);
			//!< Expects a buffer for each component
			//!< returns -1 = error, 0 = OK,
			//!< 1=last written (threshold reached)
    void		close();

protected:

    ostream&		strm_;
    const char*		errmsg_;
    unsigned long	thrbytes_;

    void		writeHdr(const CBVSInfo&);
    void		putExplicits(const CBVSInfo::ExplicitInfo&,
				     unsigned char*);
    void		writeComps(const CBVSInfo&);
    void		writeGeom(const CBVSInfo&);


private:

    streampos		geomfo;
    unsigned long	nrbytes;
    int			compnr;
    int			trcnr;
    bool		strmclosed;

    const CBVSInfo::ExplicitData*		expldat;
    ObjectSet<CBVSInfo::SurvGeom::InlineInfo>	inldata;

};


#endif
