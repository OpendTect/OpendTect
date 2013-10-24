#ifndef posinfo2dsurv_h
#define posinfo2dsurv_h

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
#include "threadlock.h"
#include "callback.h"
#include <utility>

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

typedef Pos::Index_Type Index_Type;
typedef std::pair<Index_Type,Index_Type> Index_Type_Pair;


mExpClass(Basic) Line2DKey : public Index_Type_Pair
{
public:

    typedef Index_Type	IdxType;

			Line2DKey( int lsid=-1, int lineid=-1 )
			    : Index_Type_Pair(lsid,lineid)	{}

    bool		operator ==( const Line2DKey& oth ) const
   			{ return first == oth.first && second == oth.second; }
    bool		operator !=( const Line2DKey& oth ) const
			{ return !( operator==(oth) ); }

    inline IdxType&	lsID()		{ return first; }
    inline IdxType	lsID() const	{ return first; }
    inline IdxType&	lineID()	{ return second; }
    inline IdxType	lineID() const	{ return second; }

    bool		isOK() const;
    void		setUdf()	{ *this = udf(); }
    bool		isUdf() const	{ return *this == udf(); }
    static const Line2DKey& udf();

    BufferString	toString() const;
    bool		fromString(const char*);

};

typedef Line2DKey GeomID;



/*!\brief Repository for 2D line geometries. */

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
    bool		getGeometry(const Line2DKey&,Line2DData&) const;
    			//!< thread safe

    void		renameLine(const char*oldnm,const char*newnm);
    void		removeLine(int lid);
    void		removeLineSet(int lsid);

    Line2DKey		getLine2DKey(const char* lsnm,const char* linenm) const;
    const char*		getLSFileNm(const char* lsnm) const;
    const char*		getLineFileNm(const char* lsnm,const char* lnm) const;

    bool		readDistBetwTrcsStats(const char* linemn,float& max,
	    				      float& median) const;

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
    mutable Threads::Lock lock_;

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

    mGlobal(Basic) friend const Survey2D& ::S2DPOS();

    			Survey2D();

public:

    			~Survey2D();

};


} // namespace PosInfo




#endif

