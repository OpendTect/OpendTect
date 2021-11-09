#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
________________________________________________________________________


-*/

#include "wellmod.h"
#include "wellwriteaccess.h"
#include "wellio.h"
#include "od_iosfwd.h"
class IOObj;
class DataBuffer;
class ascostream;


namespace Well
{
class Data;
class Log;

/*!\brief Writes Well::Data to OpendTect file storage. */

mExpClass(Well) odWriter : public odIO
			 , public WriteAccess
{ mODTextTranslationClass(Well::odWriter);
public:

			odWriter(const IOObj&,const Data&,uiString& errmsg);
			odWriter(const char* fnm,const Data&,uiString& errmsg);

    bool		put() const;

    virtual bool	putInfoAndTrack() const;
    virtual bool	putTrack() const;
    virtual bool	putLogs() const;
    virtual bool	putMarkers() const;
    virtual bool	putD2T() const;
    virtual bool	putCSMdl() const;
    virtual bool	putDispProps() const;
    virtual bool	putLog(const Log&) const;
    bool                putDefLogs() const override;
    virtual bool	swapLogs(const Log&,const Log&) const;
    virtual bool	renameLog(const char* oldnm, 
	    			  const char* newnm) override;

    virtual const uiString& errMsg() const	{ return odIO::errMsg(); }

    bool		putInfoAndTrack(od_ostream&) const;
    bool		putMarkers(od_ostream&) const;
    bool		putD2T(od_ostream&) const;
    bool		putCSMdl(od_ostream&) const;
    bool		putDispProps(od_ostream&) const;

    void		setBinaryWriteLogs( bool yn )	{ binwrlogs_ = yn; }

    static const char*	sKeyLogStorage()		{ return "Log storage";}

protected:

    bool		binwrlogs_;

    virtual bool	isFunctional() const;

    bool		putLog(od_ostream&,const Log&,
				  const DataBuffer* databuf = nullptr) const;
    int			getLogIndex(const char* lognm ) const;
    bool		wrLogHdr(od_ostream&,const Log&) const;
    bool		wrLogData(od_ostream&,const Log&,
				  const DataBuffer* databuf = nullptr) const;
    DataBuffer*		getLogBuffer(od_istream&) const;
    bool		wrHdr(od_ostream&,const char*) const;
    bool		putTrack(od_ostream&) const;
    bool		doPutD2T(bool) const;
    bool		doPutD2T(od_ostream&,bool) const;

private:

    void		init();

    void		setStrmErrMsg(od_stream&,const uiString&) const;
    uiString		startWriteStr() const;

};


}; // namespace Well

