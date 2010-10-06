#ifndef surv2dgeom_h
#define surv2dgeom_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2010
 RCS:		$Id: surv2dgeom.h,v 1.4 2010-10-06 08:51:52 cvssatyaki Exp $
________________________________________________________________________

-*/
 
 
#include "posinfo2d.h"
#include "separstr.h"
class IOPar;
class FilePath;
class BufferStringSet;


namespace PosInfo
{

/*!\brief Repository for 2D line geometries

  You can access it using S2DPOS() (or PosInfo::POS2DAdmin()).
 
 */

mClass Survey2D : public CallBacker
{
public:

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
    bool		hasLineSet(int lsidx) const;
    bool		hasLine(int lid,int lsid) const;
    void		getLineIDs(TypeSet<int>&,int lsidx) const;

    int			curLineSetID() const;
    void		setCurLineSet(int lsid) const;
    int			getLineSetIdx(int lsid) const;
    int			getLineIdx(int lineid) const;

    bool		getGeometry(int lidx,Line2DData&) const;

    void		removeLine(int lidx);
    void		removeLineSet(int lsidx);

protected:
    int			getNewID(IOPar&);
    void		updateMaxID(int,IOPar&);
private:

    FilePath&		basefp_;
    FilePath&		lsfp_;
    BufferString	lsnm_;
    IOPar&		lsindex_;
    IOPar&		lineindex_;

    void		readIdxFiles();
    static void		readIdxFile(const char*,IOPar&);
    void		writeIdxFile(bool) const;
    void		getKeys(const IOPar&,BufferStringSet&) const;
    void		getIDs(const IOPar&,TypeSet<int>&) const;
    BufferString	getNewStorageName(const char*,const FilePath&,
	    				  const IOPar&) const;

    mGlobal friend Survey2D&	POS2DAdmin();

    			Survey2D();
public:
    			~Survey2D();

};

mGlobal Survey2D& POS2DAdmin();

} // namespace PosInfo


inline mGlobal const PosInfo::Survey2D& S2DPOS()
{ return const_cast<PosInfo::Survey2D&>( PosInfo::POS2DAdmin() ); }


#endif
