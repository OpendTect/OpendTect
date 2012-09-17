#ifndef progressmeter_h
#define progressmeter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl / Bert Bril
 Date:          07-10-1999
 RCS:           $Id: progressmeter.h,v 1.17 2011/10/19 06:33:34 cvskris Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
#include "thread.h"

class Task;

/*!Is an interface where processes can report their progress. */
mClass ProgressMeter
{
public:
    virtual		~ProgressMeter()		{}
    virtual void	setStarted()			{}
    virtual void	setFinished()			{}

    virtual od_int64	nrDone() const			{ return -1; }
    virtual void	setName(const char*)		{}
    virtual void	setTotalNr(od_int64)		{}
    virtual void	setNrDone(od_int64)		{}
    virtual void	setNrDoneText(const char*)	{}
    virtual void	setMessage(const char*)		{}

    virtual void	operator++()			= 0;
};


/*!\brief Textual progress indicator for batch programs. */

mClass TextStreamProgressMeter : public ProgressMeter
{
public:
			TextStreamProgressMeter(std::ostream&,
					unsigned short rowlen=cDefaultRowLen());
			~TextStreamProgressMeter();
    static const int	cDefaultRowLen() { return 50; }
    static const int	cNrCharsPerRow() { return 80; }

    void		setName(const char*);
    void		setStarted();
    void		setFinished();
    void		setNrDone(od_int64);
    void		setTotalNr(od_int64 t)		{ totalnr_ = t; }
    void		setMessage(const char*);

    void		operator++();
    od_int64		nrDone() const			{ return nrdone_; }

protected:
    void		reset();
    void		addProgress(int);

    std::ostream&	strm_;
    BufferString	message_;
    BufferString	name_;
    unsigned short	rowlen_;
    unsigned char	distcharidx_;
    od_int64		nrdoneperchar_;
    od_int64		nrdone_;
    od_int64		lastannotatednrdone_;
    od_int64		totalnr_;
    int 		oldtime_; 
    int 		nrdotsonline_; 
    bool		inited_;
    bool		finished_;
    Threads::Mutex	lock_;

    void		annotate(bool);
}; 


#endif
