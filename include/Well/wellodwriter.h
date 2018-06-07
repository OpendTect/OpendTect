#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2003
________________________________________________________________________


-*/

#include "wellwriteaccess.h"
#include "wellodio.h"
#include "od_iosfwd.h"
class IOObj;
class ascostream;


namespace Well
{

/*!\brief Writes Well::Data to OpendTect file storage. */

mExpClass(Well) odWriter : public odIO
			 , public WriteAccess
{ mODTextTranslationClass(Well::odWriter)
public:

			odWriter(const IOObj&,const Data&,uiString& errmsg);
			odWriter(const char* fnm,const Data&,
				 uiString& errmsg);

    bool		put() const;

    virtual bool	putInfoAndTrack() const;
    virtual bool	putLogs() const;
    virtual bool	putMarkers() const;
    virtual bool	putD2T() const;
    virtual bool	putCSMdl() const;
    virtual bool	putDispProps() const;

    virtual const uiString& errMsg() const	{ return odIO::errMsg(); }

    bool		putInfoAndTrack(od_ostream&) const;
    bool		putMarkers(od_ostream&) const;
    bool		putD2T(od_ostream&) const;
    bool		putCSMdl(od_ostream&) const;
    bool		putDispProps(od_ostream&) const;

    void		setBinaryWriteLogs( bool yn )	{ binwrlogs_ = yn; }

    static const char*	sKeyLogStorage()		{ return "Log storage";}
    uiString		sStartWriting() const	 { return tr("Start Writing"); }


protected:

    bool		binwrlogs_;

    virtual bool	isFunctional() const;

    void		setStrmErrMsg(od_stream&,const uiString&) const;
    uiString		startWriteStr() const;
    bool		putLog(od_ostream&,const Log&) const;
    bool		wrHdr(od_ostream&,const char*) const;
    bool		putTrack(od_ostream&) const;
    bool		doPutD2T(bool) const;
    bool		doPutD2T(od_ostream&,bool) const;
    void		putDepthUnit(ascostream&) const;

private:

    void		init();

};


}; // namespace Well
