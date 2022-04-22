#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format writer
________________________________________________________________________

-*/

#include "generalmod.h"
#include "cbvsio.h"
#include "cbvsinfo.h"
#include "posinfo.h"
#include "od_ostream.h"

template <class T> class DataInterpreter;
class TraceData;


/*!\brief Writer for CBVS format

Works on an ostream that will be deleted on destruction, or when finished.

For the inline/xline info, you have two choices:
1) if you know you have a fully rectangular and regular survey, you can
   set this in the SurvGeom.
2) if this is not the case, or you don't know whether this will be the case,
   you will have to provide the BinID in the PosAuxInfo.

*/

mExpClass(General) CBVSWriter : public CBVSIO
{
public:

			CBVSWriter(od_ostream*,const CBVSInfo&,
				   const PosAuxInfo*,CoordPol cp=InAux);
			//!< If info.posauxinfo has a true, the PosAuxInfo
			//!< cannot be null. The relevant field(s) should then
			//!< be filled before the first put() of any position
			CBVSWriter(od_ostream*,const CBVSWriter&,
				   const CBVSInfo&);
			//!< For usage in CBVS pack
			~CBVSWriter();

    unsigned long	byteThreshold() const	{ return thrbytes_; }		
			//!< The default is unlimited
    void		setByteThreshold( unsigned long n )
			    { thrbytes_ = n; }		
    void		forceLineStep( const BinID& stp )
			    { forcedlinestep_ = stp; }
    void		forceNrTrcsPerPos( int nr )
			    { nrtrcsperposn_ = nr; nrtrcsperposn_status_ = 0; }
    void		forceTrailer( bool yn=true )
			    { forcetrailer_ = yn; }

    int			put(void**,int offs=0);
    int			put(const TraceData&,int offs=0);
			//!< Expects a buffer for each component
			//!< returns -1 = error, 0 = OK,
			//!< 1=not written (threshold reached)
    void		close() override	{ doClose( true ); }
    void		ciaoForNow()		{ doClose(false); }
			//!< closes as if final close but doesn't
			//!< actually close stream. Makes result readable.
    const CBVSInfo::SurvGeom& survGeom() const	{ return survgeom_; }
    const PosAuxInfoSelection& auxInfoSel()	{ return auxinfosel_; }

protected:

    od_ostream&		strm_;
    unsigned long	thrbytes_;
    int			auxnrbytes_;
    bool		input_rectnreg_;
    int*		nrbytespersample_;
    BinID		forcedlinestep_;
    bool		forcetrailer_;

    void		writeHdr(const CBVSInfo&);
    void		putAuxInfoSel(unsigned char*) const;
    void		writeComps(const CBVSInfo&);
    void		writeGeom();
    void		doClose(bool);

    void		getRealGeometry();
    bool		writeTrailer();


private:

    od_stream::Pos	geomsp_; //!< file offset of geometry data
    int			trcswritten_;
    BinID		prevbinid_;
    bool		file_lastinl_;
    int			nrtrcsperposn_;
    int			nrtrcsperposn_status_;
    int			checknrtrcsperposn_;
    PosAuxInfoSelection auxinfosel_;
    CBVSInfo::SurvGeom	survgeom_;

    const PosAuxInfo*	auxinfo_;

    void		init(const CBVSInfo&);
    void		getBinID();
    void		newSeg(bool);
    bool		writeAuxInfo();

};


