#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "progressmeter.h"
#include "bufstring.h"
#include "threadlock.h"
#include "callback.h"
#include "od_iosfwd.h"
class Task;


/*!\brief ProgressMeter that helps unifying different tasks */

mExpClass(Basic) ProgressRecorder : public ProgressMeter
{
public:

			ProgressRecorder();
			~ProgressRecorder();
    void		reset();

    void		setFrom(const Task&);

    void		setForwardTo(ProgressMeter*);
    void		setStarted() override;
    void		setFinished() override;
    void		setName(const char*) override;
    void		setTotalNr(od_int64) override;
    void		setNrDone(od_int64) override;
    void		setMessage(const uiString&) override;
    void		setMessage(const uiString&,bool printtolog);
    void		setNrDoneText(const uiString&) override;
    void		operator++() override;

    const char*		name() const;
    od_int64		nrDone() const override;
    od_int64		totalNr() const;
    uiString		message() const;
    uiString		nrDoneText() const;
    bool		isStarted() const;
    bool		isFinished() const;
    ProgressMeter*	forwardTo() const;

    void		skipProgress(bool) override;

protected:

    BufferString	name_;
    od_int64		nrdone_;
    od_int64		totalnr_;
    uiString		message_;
    uiString		nrdonetext_;
    bool		isstarted_;
    bool		isfinished_;
    ProgressMeter*	forwardto_;

    Threads::Lock&	lock_;

};


/*!\brief Textual progress indicator for batch programs. */

mExpClass(Basic) TextStreamProgressMeter : public ProgressMeter
{
public:

			TextStreamProgressMeter(od_ostream&,
					unsigned short rowlen=cDefaultRowLen());
			~TextStreamProgressMeter();

    static int		cDefaultRowLen() { return 50; }
    static int		cNrCharsPerRow() { return 80; }

    void		setName(const char*) override;
    void		setStarted() override;
    void		setFinished() override;
    void		setNrDone(od_int64) override;
    void		setTotalNr(od_int64 t) override
			{
			    Threads::Locker lock( lock_ );
			    totalnr_ = t;
			}

    void		setMessage(const uiString&) override;
    void		printMessage(const uiString&);

			/*!<This setting will not reset unless you call it.*/
    void		skipProgress( bool yn ) override { skipprog_ = yn; }

    void		operator++() override;
    od_int64		nrDone() const override		{ return nrdone_; }

    void		reset();

protected:

    void		addProgress(int);

    od_ostream&		strm_;
    uiString		message_;
    BufferString	name_;
    unsigned short	rowlen_;
    unsigned char	distcharidx_;
    od_int64		nrdoneperchar_;
    od_int64		nrdone_;
    od_int64		lastannotatednrdone_;
    od_int64		totalnr_;
    int			oldtime_;
    int			nrdotsonline_;
    bool		inited_;
    bool		finished_;
    Threads::Lock	lock_;
    bool		skipprog_;

    void		annotate(bool);
};
