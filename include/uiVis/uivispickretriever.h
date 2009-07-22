#ifndef uivispickretriever_h
#define uivispickretriever_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Mar 2002
 RCS:           $Id: uivispickretriever.h,v 1.4 2009-07-22 16:01:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "pickretriever.h"

namespace visSurvey { class Scene; }

mClass uiVisPickRetriever : public PickRetriever
{
public:
    			uiVisPickRetriever();
    bool		enable(const TypeSet<int>* allowedscenes);
    NotifierAccess*	finished()		{ return &finished_; }

    void		reset();
    bool		success() const		{ return status_==Success; }
    bool		waiting() const		{ return status_==Waiting; }
    const Coord3&	getPos() const		{ return pickedpos_; }
    int			getSceneID() const	{ return pickedscene_; }
    			
    void		addScene(visSurvey::Scene*);
    void		removeScene(visSurvey::Scene*);

protected:
				~uiVisPickRetriever();
    void			pickCB(CallBacker*);

    ObjectSet<visSurvey::Scene>	scenes_;
    TypeSet<int>		allowedscenes_;

    enum Status			{ Idle, Waiting, Failed, Success } status_;
    Coord3			pickedpos_;
    int				pickedscene_;
    Notifier<uiVisPickRetriever> finished_;
};

#endif
