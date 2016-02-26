#ifndef progressmeterimpl_h
#define progressmeterimpl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl / Bert Bril
 Date:          07-10-1999
 RCS:           $Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "progressmeter.h"
#include "bufstring.h"
#include "threadlock.h"
#include "callback.h"
#include "od_iosfwd.h"


/*!\brief ProgressMeter recording the progress. Can also forward to another. */

mExpClass(Basic) ProgressRecorder : public ProgressMeter
{
public:

			ProgressRecorder();
			~ProgressRecorder();

    void		reset();

    BufferString	name_;
    uiString		message_;
    uiString		nrdonetext_;
    bool		isstarted_;
    bool		isfinished_;
    od_int64		nrdone_;
    od_int64		totalnr_;
    ProgressMeter*	forwardto_;

    virtual void	setStarted();
    virtual void	setFinished();
    virtual od_int64	nrDone() const			{ return nrdone_; }
    virtual void	setName(const char*);
    virtual void	setTotalNr(od_int64);
    virtual void	setNrDone(od_int64);
    virtual void	setMessage(const uiString&);
    virtual void	setNrDoneText(const uiString&);
    virtual void	operator++();

protected:

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

    void		setName(const char*);
    void		setStarted();
    void		setFinished();
    void		setNrDone(od_int64);
    void		setTotalNr(od_int64 t)
			{
			    Threads::Locker lock( lock_ );
			    totalnr_ = t;
			}

    void		setMessage(const uiString&);

			/*!<This setting will not reset unless you call it.*/
    void		skipProgress( bool yn )		{ skipprog_ = yn; }

    void		operator++();
    od_int64		nrDone() const			{ return nrdone_; }

protected:

    void		reset();
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


#endif
