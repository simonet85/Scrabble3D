#pragma once

// Short audible notification when a player places all pieces of the rack (Scrabble/Bingo).
// Windows: four rising notes played from a worker thread; other platforms: the system beep.
// Needs no audio module, so the application keeps its small footprint.
void playScrabbleSound();
