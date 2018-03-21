#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl / Bert
 Date:          07-10-1999
________________________________________________________________________

-*/

#include "basicmod.h"
#include "progressmeter.h"
#include "bufstring.h"
#include "od_iosfwd.h"
class Task;
namespace Threads { class Lock; }


/*!\brief ProgressMeter that helps unifying different tasks */

mExpClass(Basic) ProgressRecorder : public ProgressMeter
{
public:

			ProgressRecorder();
			~ProgressRecorder();
    void		reset();

    void		setFrom(const Task&);

    void		setForwardTo(ProgressMeter*);
    virtual void	setStarted();
    virtual void	setFinished();
    virtual void	setName(const char*);
    virtual void	setTotalNr(od_int64);
    virtual void	setNrDone(od_int64);
    virtual void	setNrDoneText(const uiString&);
    virtual void	setMessage(const uiString&);
    void		setMessage(const uiString&,bool printtolog);

    virtual void	operator++();

    const char*		name() const;
    virtual od_int64	nrDone() const;
    od_int64		totalNr() const;
    uiString		message() const;
    uiString		nrDoneText() const;
    bool		isStarted() const;
    bool		isFinished() const;
    ProgressMeter*	forwardTo() const;

    virtual void	skipProgress(bool);

protected:

    BufferString	name_;
    od_int64		nrdone_;
    od_int64		totalnr_;
    uiString		message_;
    uiString		nrdonetext_;
    bool		isstarted_;
    bool		isfinished_;
    ProgressMeter*	forwardto_;
    bool		skipprog_;

    Threads::Lock&	lock_;

};


/*!\brief Textual progress indicator for batch programs. */

mExpClass(Basic) TextStreamProgressMeter : public ProgressMeter
{
public:

			TextStreamProgressMeter(od_ostream&,
					od_uint16 rowlen=cDefaultRowLen());
			~TextStreamProgressMeter();

    static od_uint16	cDefaultRowLen() { return 50; }
    static od_uint16	cNrCharsPerRow() { return 80; }

    void		setName(const char*);
    void		setStarted();
    void		setFinished();
    void		setNrDone(od_int64);
    void		setTotalNr(od_int64 t);

    void		setMessage(const uiString&);
    virtual void	printMessage(const uiString&);

			/*!<This setting will not reset unless you call it.*/
    void		skipProgress( bool yn )		{ skipprog_ = yn; }

    void		operator++();
    od_int64		nrDone() const			{ return nrdone_; }

    void		reset();

protected:

    void		addProgress(int);

    od_ostream&		strm_;
    uiString		message_;
    BufferString	name_;
    od_uint16		rowlen_;
    unsigned char	distcharidx_;
    od_int64		nrdoneperchar_;
    od_int64		nrdone_;
    od_int64		lastannotatednrdone_;
    od_int64		totalnr_;
    int			oldtime_;
    int			nrdotsonline_;
    bool		inited_;
    bool		finished_;
    Threads::Lock&	lock_;
    bool		skipprog_;

    void		annotate(bool);

};
