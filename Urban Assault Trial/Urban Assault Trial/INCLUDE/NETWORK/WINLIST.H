/*** Windows Listenzeug. Windows hat ja kein Listenhandling. Oh Mann! ***/
#ifndef WINLIST_H
#define WINLIST_H

struct WinNode {

    struct WinNode *ln_Succ;
    struct WinNode *ln_Pred;
};


struct WinList {

    struct WinNode *lh_Head;
    struct WinNode *lh_Tail;
    struct WinNode *lh_TailPred;
};

#define IsListEmpty(x) \
        ( ((x)->lh_TailPred) == (struct Node *)(x) )


#endif
