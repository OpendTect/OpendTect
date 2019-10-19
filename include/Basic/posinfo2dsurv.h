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

class BufferStringSet;
namespace File { class Path; }
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

    typedef int		idx_type;

			Line2DKey( pos_type lsid=-1, pos_type lineid=-1 )
			    : IdxPair(lsid,lineid)	{}

    inline bool		operator ==( const Line2DKey& oth ) const
			{ return IdxPair::operator==(oth); }
    inline bool		operator !=( const Line2DKey& oth ) const
			{ return !( operator==(oth) ); }

    inline pos_type&	lsID()		{ return first(); }
    inline pos_type	lsID() const	{ return first(); }
    inline pos_type&	lineID()	{ return second(); }
    inline pos_type	lineID() const	{ return second(); }

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

    mUseType( Line2DKey,	pos_type );

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
    const char*		getLineSet(pos_type lsid) const;
    const char*		getLineName(pos_type lineid) const;
    pos_type		getLineSetID(const char*) const;
    pos_type		getLineID(const char*) const;
    bool		hasLineSet(pos_type lsid) const;
    bool		hasLine(pos_type lineid,pos_type lsid=-1) const;
    void		getLineIDs(TypeSet<pos_type>&,pos_type lsid) const;
    void		getLines(BufferStringSet&,pos_type lsid) const;

    pos_type		curLineSetID() const;
    void		setCurLineSet(pos_type lsid) const;

    bool		getGeometry(pos_type lid,Line2DData&) const;
    bool		getGeometry(const Line2DKey&,Line2DData&) const;
			//!< thread safe

    void		renameLine(const char*oldnm,const char*newnm);
    void		removeLine(pos_type lid);
    void		removeLineSet(pos_type lsid);

    Line2DKey		getLine2DKey(const char* lsnm,const char* linenm) const;
    const char*		getLSFileNm(const char* lsnm) const;
    const char*		getLineFileNm(const char* lsnm,const char* lnm) const;

    bool		readDistBetwTrcsStats(const char* linemn,float& max,
					      float& median) const;

protected:

    pos_type		getNewID(IOPar&);
    void		updateMaxID(pos_type,IOPar&);

private:

    typedef int		idx_type;
    File::Path&		basefp_;
    File::Path&		lsfp_;
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
    void		getIDs(const IOPar&,TypeSet<pos_type>&) const;
    BufferString	getNewStorageName(const char*,const File::Path&,
					  const IOPar&) const;
    idx_type		getLineSetIdx(pos_type lsid) const;
    idx_type		getLineIdx(pos_type lineid) const;

    mGlobal(Basic) friend const Survey2D& ::S2DPOS();

			Survey2D();

public:

			~Survey2D();

};


} // namespace PosInfo
