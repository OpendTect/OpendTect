#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"
#include "wellreadaccess.h"
#include "wellio.h"

#include "od_iosfwd.h"
#include "ranges.h"
#include "sets.h"

class BufferStringSet;
class IOObj;


namespace Well
{
class Data;
class Log;

/*!\brief Reads Well::Data from OpendTect file store  */

mExpClass(Well) odReader : public odIO
			 , public Well::ReadAccess
{ mODTextTranslationClass(Well::odReader);
public:

			odReader(const IOObj&,Data&,uiString& errmsg);
			odReader(const char* fnm,Data&,uiString& errmsg);
    virtual		~odReader();

    bool		get() const override		{ return true; }

    bool		getInfo() const override;
    bool		getTrack() const override;
    bool		getLogs(bool needjustinfo=false) const override;
    bool		getMarkers() const override; //needs to read Track too
    bool		getD2T() const override;
    bool		getCSMdl() const override;
    bool		getDispProps() const override;
    bool		getLog(const char* lognm) const override;
    void		getLogInfo(BufferStringSet& lognms) const override;
    bool		getDefLogs() const override;

    const uiString& errMsg() const override	{ return odIO::errMsg(); }

    bool		getInfo(od_istream&) const;
    bool		addLog(od_istream&, bool needjustinfo=false) const;
    bool		getMarkers(od_istream&) const;
    bool		getD2T(od_istream&) const;
    bool		getCSMdl(od_istream&) const;
    bool		getDispProps(od_istream&) const;
    bool		getDefLogs(od_istream&) const;

protected:

    bool		getOldTimeWell(od_istream&) const;
    void		getLogInfo(BufferStringSet&,TypeSet<int>&) const;
    void		readLogData(Log&,od_istream&,int) const;
    bool		getTrack(od_istream&) const;
    bool		doGetD2T(od_istream&,bool csmdl) const;
    bool		doGetD2T(bool) const;

    static Log*		rdLogHdr(od_istream&,int&,int);

    void		setInpStrmOpenErrMsg(od_istream&) const;
    void		setStrmOperErrMsg(od_istream&,const uiString&) const;
    uiString		sCannotReadFileHeader() const;

};

} // namespace Well
