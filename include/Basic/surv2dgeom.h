#ifndef surv2dgeom_h
#define surv2dgeom_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2010
 RCS:		$Id$
________________________________________________________________________

-*/
 
 
#include "basicmod.h"
#include "posinfo2d.h"
#include "separstr.h"
#include "survgeom.h"
#include "thread.h"

class IOPar;
class FilePath;
class BufferStringSet;


namespace PosInfo
{

/*!
\ingroup Basic
\brief Geometry ID. 
*/

mExpClass(Basic) GeomID
{
public:
    		GeomID( int lsid=-1, int lineid=-1 )
		    : lsid_(lsid) ,lineid_(lineid)	{}

    int		lsid_;
    int		lineid_;

    bool	isOK() const;
    void	setUndef();

    bool	operator ==( const GeomID& a ) const
   		{ return a.lsid_ == lsid_ && a.lineid_ == lineid_; }
    bool	operator !=( const GeomID& a ) const
		{ return !( operator==(a) ); }
    BufferString toString() const;
    bool	fromString(const char*);
};


/*!
\ingroup Basic
\brief Repository for 2D line geometries.

  You can access it using S2DPOS() (or PosInfo::POS2DAdmin()).
*/

mExpClass(Basic) Survey2D : public CallBacker
{
public:

    static void		initClass();
    bool		isEmpty() const		{ return lsnm_.isEmpty(); }

    //using names
    bool		hasLineSet(const char*) const;
    bool		hasLine(const char* lnm,const char* lsnm=0) const;
    void		getLineSets( BufferStringSet& nms ) const
						{ getKeys(lsindex_,nms); }
    void		getLines(BufferStringSet&,const char* lsnm=0) const;

    const char*		curLineSet() const	{ return lsnm_.buf(); }
    void		setCurLineSet(const char*) const;

    bool		getGeometry(Line2DData&) const; //!< using lineName()
    bool		setGeometry(const Line2DData&);

    void		removeLine(const char*);
    void		removeLineSet(const char*);
    void		renameLineSet(const char*,const char*);
    
    // using ids
    const char*		getLineSet(int lsid) const;
    const char*		getLineName(int lineid) const;
    int			getLineSetID(const char*) const;
    int			getLineID(const char*) const;
    bool		hasLineSet(int lsid) const;
    bool		hasLine(int lineid,int lsid=-1) const;
    void		getLineIDs(TypeSet<int>&,int lsid) const;
    void		getLines(BufferStringSet&,int lsid) const;

    int			curLineSetID() const;
    void		setCurLineSet(int lsid) const;

    bool		getGeometry(int lid,Line2DData&) const;
    bool		getGeometry(const GeomID&,Line2DData&) const;
    			//!< thread safe

    void		renameLine(const char*oldnm,const char*newnm);
    void		removeLine(int lid);
    void		removeLineSet(int lsid);

    GeomID		getGeomID(const char* lsnm,const char* linenm) const;
    const char*		getLSFileNm(const char* lsnm) const;
    const char*		getLineFileNm(const char* lsnm,const char* lnm) const;

protected:
    int			getNewID(IOPar&);
    void		updateMaxID(int,IOPar&);

private:

    FilePath&		basefp_;
    FilePath&		lsfp_;
    BufferString	lsnm_;
    IOPar&		lsindex_;
    IOPar&		lineindex_;
    mutable BufferString curlstimestr_;
    mutable Threads::Mutex mutex_;

    void		readIdxFiles();
    bool		isIdxFileNew(const char* lsnm=0) const;
    BufferString 	getIdxTimeStamp(const char* lsnm=0) const;
    static void		readIdxFile(const char*,IOPar&);
    void		writeIdxFile(bool) const;
    void		getKeys(const IOPar&,BufferStringSet&) const;
    void		getIDs(const IOPar&,TypeSet<int>&) const;
    BufferString	getNewStorageName(const char*,const FilePath&,
	    				  const IOPar&) const;
    int			getLineSetIdx(int lsid) const;
    int			getLineIdx(int lineid) const;

    mGlobal(Basic) friend Survey2D&	POS2DAdmin();

    			Survey2D();
public:
    			~Survey2D();

};

mGlobal(Basic) Survey2D& POS2DAdmin();

} // namespace PosInfo


inline mGlobal(Basic) const PosInfo::Survey2D& S2DPOS()
{ return const_cast<PosInfo::Survey2D&>( PosInfo::POS2DAdmin() ); }



//New Stuff post 4.4 that will replace the old stuff in due course

namespace Survey
{

/*!
\ingroup Basic
\brief Geometry of a 2D Line.
*/

mExpClass(Basic) Geometry2D : public Geometry
{
public:
                   		Geometry2D();
				Geometry2D(PosInfo::Line2DData& l2d)
				:data_(l2d){}

    Coord			toCoord(const TraceID& tid) const
				{ return toCoord( tid.line_, tid.trcnr_); }
    virtual Coord		toCoord(int line, int tracenr) const;
    virtual TraceID		nearestTrace(const Coord&,float* dist) const;

    virtual bool		includes(int line, int tracenr)	const;

    bool			is2D() const		{ return true; }
    PosInfo::Line2DData&	data()			{ return data_; }
    const PosInfo::Line2DData	data() const		{ return data_; }

protected:

                    		~Geometry2D();

    PosInfo::Line2DData&	data_;
};

} // namespace Survey

#endif

