#ifndef cbvswriter_h
#define cbvswriter_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format writer
 RCS:		$Id: cbvswriter.h,v 1.8 2001-05-31 13:23:52 windev Exp $
________________________________________________________________________

-*/

#include <cbvsio.h>
#include <cbvsinfo.h>
#include <iostream>

template <class T> class DataInterpreter;


/*!\brief Writer for CBVS format

Works on an ostream that will be deleted on destruction, or when finished.

For the inline/xline info, you have two choices:
1) if you know you have a fully rectangular and regular survey, you can
   set this in the SurvGeom.
2) if this is not the case, or you don't know whether this will be the case,
   you will have to provide the BinID in the ExplicitData.

*/

class CBVSWriter : public CBVSIO
{
public:

			CBVSWriter(ostream*,const CBVSInfo&,
				   const CBVSInfo::ExplicitData* =0);
			//!< If info.explinfo has a true, the ExplicitData
			//!< is mandatory. The relevant field(s) should then be
			//!< filled before the first put() of any position
			CBVSWriter(ostream*,const CBVSWriter&,const CBVSInfo&);
			//!< For usage in CBVS pack
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
    const CBVSInfo::SurvGeom& survGeom() const	{ return survgeom; }
    const CBVSInfo::ExplicitInfo& explInfo()	{ return explinfo; }

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

    void		init(const CBVSInfo&);
    void		getBinID();
    void		newSeg();
    bool		writeExplicits();

};


#endif
