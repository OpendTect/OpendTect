#ifndef progressmeter_h
#define progressmeter_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl / Bert Bril
 Date:          07-10-1999
 RCS:           $Id: progressmeter.h,v 1.11 2007-12-05 21:44:20 cvskris Exp $
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

    virtual int		nrDone() const			{ return -1; }
    virtual void	setName(const char*)		{}
    virtual void	setTotalNr(int)			{}
    virtual void	setNrDone(int)			{}
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
    void	setNrDone(int);
    void	setMessage(const char*);

    void	operator++();
    int		nrDone() const			{ return nrdone_; }

protected:
    void	reset();
    void	addProgress(int);

    std::ostream&	strm_;
    BufferString	message_;
    BufferString	name_;
    unsigned short	rowlen_;
    unsigned char	distcharidx_;
    unsigned long	nrdoneperchar_;
    unsigned long       nrdone_;
    unsigned long	lastannotatednrdone_;
    int 		oldtime_; 
    int 		nrdotsonline_; 
    bool		inited_;
    bool		finished_;
    Threads::Mutex	lock_;

    void		annotate(bool);
}; 


#endif
