#ifndef progressmeter_h
#define progressmeter_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl / Bert Bril
 Date:          07-10-1999
 RCS:           $Id: progressmeter.h,v 1.12 2008-09-22 12:49:54 cvskris Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
#include "thread.h"

class Task;

/*!Is an interface where processes can report their progress. */
class ProgressMeter
{
public:
    virtual		~ProgressMeter()		{}
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

class TextStreamProgressMeter : public ProgressMeter
{
public:
		TextStreamProgressMeter(std::ostream&,unsigned short rowlen=50);
    		~TextStreamProgressMeter();
    void	setName(const char*);
    void	setFinished();
    void	setNrDone(od_int64);
    void	setMessage(const char*);

    void	operator++();
    od_int64	nrDone() const			{ return nrdone_; }

protected:
    void	reset();
    void	addProgress(int);

    std::ostream&	strm_;
    BufferString	message_;
    BufferString	name_;
    unsigned short	rowlen_;
    unsigned char	distcharidx_;
    od_int64		nrdoneperchar_;
    od_int64		nrdone_;
    od_int64		lastannotatednrdone_;
    int 		oldtime_; 
    int 		nrdotsonline_; 
    bool		inited_;
    bool		finished_;
    Threads::Mutex	lock_;

    void		annotate(bool);
}; 


#endif
