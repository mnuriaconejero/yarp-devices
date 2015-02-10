// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

#include "RecordManipulation.hpp"

/************************************************************************/
bool MoveGripperPort::setLeftOpenChar(const char& openLeftChar) {
    this->openLeftChar = openLeftChar;
}

/************************************************************************/
bool MoveGripperPort::setLeftCloseChar(const char& closeLeftChar) {
    this->closeLeftChar = closeLeftChar;
}

/************************************************************************/
bool MoveGripperPort::setRightOpenChar(const char& openRightChar) {
    this->openRightChar = openRightChar;
}

/************************************************************************/
bool MoveGripperPort::setRightCloseChar(const char& closeRightChar) {
    this->closeRightChar = closeRightChar;
}

/************************************************************************/
bool MoveGripperPort::threadInit() {

    w = initscr();
    //raw();  //-- raw deactivates signals and gets raw text. this we do NOT want.
    cbreak();  //-- cbreak allow ctrl-c to reach normal handler. this we DO want.
    nodelay(w, TRUE);

    return true;
}

/************************************************************************/
void MoveGripperPort::run() {
    noecho();

    int leftArmSize, rightArmSize;
    leftArmPos->getAxes(&leftArmSize);
    rightArmPos->getAxes(&rightArmSize);

    while( ! this->isStopping() ) {
        char c = getch();
        if(c == openLeftChar) {
            //CD_WARNING("openLeftChar\n");
            leftArmPos->positionMove(leftArmSize-1,1200);
        } else if(c == closeLeftChar) {
            //CD_WARNING("closeLeftChar");
            leftArmPos->positionMove(leftArmSize-1,-1200);
        } else if(c == openRightChar) {
            //CD_WARNING("openRightChar");
            rightArmPos->positionMove(rightArmSize-1,1200);
        } else if(c == closeRightChar) {
            //CD_WARNING("closeRightChar");
            rightArmPos->positionMove(rightArmSize-1,-1200);
        }
    }
    endwin();

}

/************************************************************************/
