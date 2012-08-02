
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2012 Thomas Perl <m@thp.io>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 **/


#include <QApplication>
#include <QThread>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "psmove.h"
#include "psmove_tracker.h"

class Orientation : public QThread
{
    Q_OBJECT

    signals:
        void newposition(qreal scale, qreal x, qreal y, qreal trigger);
        void newcolor(int r, int g, int b);
        void newimage(void *image);
        void backup_frame();
        void restore_frame();

    public:
        Orientation() : QThread() {}

        void run()
        {
            PSMove *move = psmove_connect();
            int quit = 0;

            if (move == NULL) {
                fprintf(stderr, "Could not connect to controller.\n");
                QApplication::quit();
            }

            PSMoveTracker *tracker = psmove_tracker_new();
            while (psmove_tracker_enable(tracker, move) != Tracker_CALIBRATED) {
                // Wait until calibration is done
            }

            while (!quit) {
                while (psmove_poll(move)) {
                    if (psmove_get_buttons(move) & Btn_PS) {
                        quit = 1;
                        break;
                    }

                    if (psmove_get_buttons(move) & Btn_SELECT) {
                        emit backup_frame();
                    }

                    if (psmove_get_buttons(move) & Btn_START) {
                        emit restore_frame();
                    }

                    if (psmove_get_buttons(move) & Btn_MOVE) {
                        emit newcolor(0, 0, 0);
                    }

                    if (psmove_get_buttons(move) & Btn_CROSS) {
                        emit newcolor(0, 0, 255);
                    }

                    if (psmove_get_buttons(move) & Btn_SQUARE) {
                        emit newcolor(255, 255, 0);
                    }

                    if (psmove_get_buttons(move) & Btn_TRIANGLE) {
                        emit newcolor(0, 255, 0);
                    }

                    if (psmove_get_buttons(move) & Btn_CIRCLE) {
                        emit newcolor(255, 0, 0);
                    }

                    int x, y, radius;
                    psmove_tracker_get_position(tracker, move, &x, &y, &radius);
                    emit newposition(radius, x, y,
                            (qreal)psmove_get_trigger(move) / 255.);
                }

                psmove_tracker_update_image(tracker);
                psmove_tracker_update(tracker, NULL);

                emit newimage(psmove_tracker_get_image(tracker));

                unsigned char r, g, b;
                psmove_tracker_get_color(tracker, move, &r, &g, &b);
                psmove_set_leds(move, r, g, b);
                psmove_update_leds(move);
            }

            psmove_tracker_free(tracker);
            psmove_disconnect(move);
            QApplication::quit();
        }
};
