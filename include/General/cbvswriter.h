#ifndef cbvswriter_h
#define cbvswriter_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format writer
 RCS:		$Id: cbvswriter.h,v 1.6 2001-04-05 16:21:35 bert Exp $
________________________________________________________________________

-*/

#include <cbvsio.h>
#include <cbvsinfo.h>
#include <iostream.h>
template <class T> class DataInterpreter;


/*!\brief Writer for CBVS format

Works on an ostream that will be deleted on destruction, or when finished.

*/

class CBVSWriter : public CBVSIO
{
public:

			CBVSWriter(ostream*,const CBVSInfo&,
				   const CBVSInfo::ExplicitData* =0);
			//!< If info.explinfo has a true, the ExplicitData
			//!< is mandatory. The relevant field(s) should then be
			//!< filled before the first put() of any position
			~CBVSWriter();

    unsigned long	byteThreshold() const	{ return thrbytes_; }		
			//!< The default is unlimited
    void		setByteThreshold( unsigned long n )
						{ thrbytes_ = n; }		

    int			put(void**);
			//!< Expects a buffer for each component
			//!< returns -1 = error, 0 = OK,
			//!< 1=not written (threshold reached)
    void		close();
			//!< has no effect (but doesn't hurt) if put() returns 1

protected:

    ostream&		strm_;
    unsigned long	thrbytes_;

    void		writeHdr(const CBVSInfo&);
    void		putExplicits(unsigned char*) const;
    void		writeComps(const CBVSInfo&);
    void		writeGeom();

    void		getRealGeometry();
    bool		writeTrailer();


private:

    streampos		geomfo; //!< file offset of geometry data
    bool		finishing_inline;
    int			trcswritten;
    int			previnl;

    int			nrtrcsperposn;
    CBVSInfo::ExplicitInfo explinfo;
    CBVSInfo::SurvGeom	survgeom;

    const CBVSInfo::ExplicitData*		expldat;
    ObjectSet<CBVSInfo::SurvGeom::InlineInfo>	inldata;

    void		getBinID();
    void		newSeg();
    bool		writeExplicits();

};


#endif
