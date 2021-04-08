#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2010
________________________________________________________________________

-*/


#include "basicmod.h"
#include "posinfo2d.h"
#include "separstr.h"
#include "survgeom.h"
#include "threadlock.h"
#include "callback.h"
#include "idxpair.h"

class FilePath;
class BufferStringSet;
namespace PosInfo { class Survey2D; }


/*!\brief Your read-access to the 2D line geometry database */

mGlobal(Basic) const PosInfo::Survey2D& S2DPOS();


namespace PosInfo
{

/*!\brief Your R/W access to the 2D line geometry database */

inline mGlobal(Basic) Survey2D& POS2DAdmin()
{
    return const_cast<Survey2D&>( S2DPOS() );
}



/*!\brief Key holding ID for both lineset and line. */


mExpClass(Basic) Line2DKey : public IdxPair
{
public:

			Line2DKey( int lsid=-1, int lineid=-1 )
			    : IdxPair(lsid,lineid)	{}

    inline bool		operator ==( const Line2DKey& oth ) const
			{ return IdxPair::operator==(oth); }
    inline bool		operator !=( const Line2DKey& oth ) const
			{ return !( operator==(oth) ); }

    inline IdxType&	lsID()		{ return first; }
    inline IdxType	lsID() const	{ return first; }
    inline IdxType&	lineID()	{ return second; }
    inline IdxType	lineID() const	{ return second; }

    bool		isOK() const;	//!< true if a line exist with this key

    const char*		toString() const;
    bool		fromString(const char*);

    bool		haveLSID() const;
    bool		haveLineID() const;

    bool		isUdf() const;
    static const Line2DKey& udf();

};

typedef Line2DKey GeomID;



/*!\brief Repository for 2D line geometries. */

mExpClass(Basic) Survey2D : public CallBacker
{
public:

    typedef Line2DKey::IdxType	IdxType;

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
    const char*		getLineSet(IdxType lsid) const;
    const char*		getLineName(IdxType lineid) const;
    IdxType		getLineSetID(const char*) const;
    IdxType		getLineID(const char*) const;
    bool		hasLineSet(IdxType lsid) const;
    bool		hasLine(IdxType lineid,IdxType lsid=-1) const;
    void		getLineIDs(TypeSet<IdxType>&,IdxType lsid) const;
    void		getLines(BufferStringSet&,IdxType lsid) const;

    IdxType		curLineSetID() const;
    void		setCurLineSet(IdxType lsid) const;

    bool		getGeometry(IdxType lid,Line2DData&) const;
    bool		getGeometry(const Line2DKey&,Line2DData&) const;
			//!< thread safe

    void		renameLine(const char*oldnm,const char*newnm);
    void		removeLine(IdxType lid);
    void		removeLineSet(IdxType lsid);

    Line2DKey		getLine2DKey(const char* lsnm,const char* linenm) const;
    const char*		getLSFileNm(const char* lsnm) const;
    const char*		getLineFileNm(const char* lsnm,const char* lnm) const;

    bool		readDistBetwTrcsStats(const char* linemn,float& max,
					      float& median) const;

protected:

    IdxType		getNewID(IOPar&);
    void		updateMaxID(IdxType,IOPar&);

private:

    FilePath&		basefp_;
    FilePath&		lsfp_;
    BufferString	lsnm_;
    IOPar&		lsindex_;
    IOPar&		lineindex_;
    mutable BufferString curlstimestr_;
    mutable Threads::Lock lock_;

    void		readIdxFiles();
    bool		isIdxFileNew(const char* lsnm=0) const;
    BufferString	getIdxTimeStamp(const char* lsnm=0) const;
    static void		readIdxFile(const char*,IOPar&);
    void		writeIdxFile(bool) const;
    void		getKeys(const IOPar&,BufferStringSet&) const;
    void		getIDs(const IOPar&,TypeSet<IdxType>&) const;
    BufferString	getNewStorageName(const char*,const FilePath&,
					  const IOPar&) const;
    int			getLineSetIdx(IdxType lsid) const;
    int			getLineIdx(IdxType lineid) const;

    mGlobal(Basic) friend const Survey2D& ::S2DPOS();

			Survey2D();

public:

			~Survey2D();

};


} // namespace PosInfo

