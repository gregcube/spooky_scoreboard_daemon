#ifndef _X11_H
#define _X11_H

void x11Init();
void openPlayerWindows();
void closePlayerWindows();
void showPlayerWindow(int index);
void hidePlayerWindow(int index);
void drawPlayerWindow(int index);
void runTimer(int secs, int index);

#endif
