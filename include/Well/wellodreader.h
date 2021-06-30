#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2003
________________________________________________________________________


-*/

#include "wellreadaccess.h"
#include "wellodio.h"
#include "od_iosfwd.h"
class IOObj;


namespace Well
{

/*!\brief Reads Well::Data from OpendTect file store  */

mExpClass(Well) odReader : public odIO
			 , public Well::ReadAccess
{ mODTextTranslationClass(Well::odReader)
public:

			odReader(const IOObj&,Data&,uiString& errmsg);
			odReader(const char* fnm,Data&,uiString& errmsg);

    virtual bool	getInfo() const;
    virtual bool	getTrack() const;
    virtual bool	getLogs(bool needjustinfo=false) const;
    virtual bool	getMarkers() const;	//needs to read Track too
    virtual bool	getD2T() const;
    virtual bool	getCSMdl() const;
    virtual bool	getDispProps() const;
    virtual bool	getLog(const char* lognm) const;
    virtual void	getLogNames(BufferStringSet&) const;
    virtual void	getLogInfo(ObjectSet<IOPar>&) const;

    virtual const uiString& errMsg() const	{ return odIO::errMsg(); }

    bool		getInfo(od_istream&) const;
    bool		addLog(od_istream&, bool needjustinfo=false) const;
    bool		getMarkers(od_istream&) const;
    bool		getD2T(od_istream&) const;
    bool		getCSMdl(od_istream&) const;
    bool		getDispProps(od_istream&) const;

protected:

    void		readLogData(Log&,od_istream&,int) const;
    bool		gtTrack(od_istream&,float) const;
    bool		doGetD2T(od_istream&,bool csmdl) const;
    bool		doGetD2T(bool) const;
    void		adjustTrackIfNecessary(bool frommarkers=false) const;
    void		getLogNames(BufferStringSet&,TypeSet<int>&) const;

    static Log*		rdLogHdr(od_istream&,int&,int,IOPar&);

    void		setInpStrmOpenErrMsg(od_istream&) const;
    void		setStrmOperErrMsg(od_istream&,const uiString&) const;
    uiString		sCannotReadFileHeader() const;

};

}; // namespace Well
