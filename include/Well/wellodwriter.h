#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"

#include "uistring.h"
#include "wellwriteaccess.h"
#include "wellio.h"

#include "od_iosfwd.h"

class DataBuffer;
class IOObj;
class uiString;
class ascostream;


namespace Well
{
class Data;
class Log;

/*!\brief Writes Well::Data to OpendTect file storage. */

mExpClass(Well) odWriter : public odIO
			 , public WriteAccess
{
mODTextTranslationClass(odWriter)
public:
			odWriter(const IOObj&,const Data&,uiString& errmsg);
			odWriter(const char* fnm,const Data&,uiString& errmsg);
			~odWriter();

    static const char*	sKeyLogStorage()		{ return "Log storage";}

private:
    bool		isFunctional() const override;
    bool		canRenameLogs() const override		{ return true; }
    bool		canWriteInParallel() const override	{ return true; }
    bool		needsInfoAndTrackCombined() const override
			{ return true; }

    bool		put() const override;
    bool		putInfo() const override;
    bool		putTrack() const override;
    bool		putCSMdl() const override;
    bool		putD2T() const override;
    bool		putMarkers() const override;
    bool		putLogs() const override;
    bool		putLog(const Log&) const override;
    bool		putDefLogs() const override;
    bool		putDispProps() const override;
    bool		renameLog(const char* oldnm,
				  const char* newnm) override;

    const uiString&	errMsg() const override { return odIO::errMsg(); }

    bool		putInfo(od_ostream&) const;
    bool		putTrack(od_ostream&) const;
    bool		putMarkers(od_ostream&) const;
    bool		putDefLogs(od_ostream&) const;
    bool		putD2T(od_ostream&) const;
    bool		putCSMdl(od_ostream&) const;
    bool		putDispProps(od_ostream&) const;

    bool		binwrlogs_;

    bool		putLog(od_ostream&,const Log&,int bintype,
			       const DataBuffer* databuf =nullptr) const;
    int			getLogIndex(const char* lognm) const;
    bool		wrLogHdr(const Log&,int bintype,od_ostream&) const;
    bool		wrLogData(const Log&,int bintype,const DataBuffer*,
				  od_ostream&) const;
    DataBuffer*		getLogBuffer(od_istream&) const;
    bool		wrHdr(od_ostream&,const char*) const;
    bool		doPutD2T(bool) const;
    bool		doPutD2T(od_ostream&,bool) const;

    void		init();

    void		setStrmErrMsg(od_stream&,const uiString&) const;
    uiString		startWriteStr() const;

};

} // namespace Well
