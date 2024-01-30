#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <curses.h>
#include <time.h>

// Types
typedef enum screenCharType
{
  spawner,       // This is the character that moves down
  stuckStatic,   // This means the character just stays there
  stuckChanging, // Some characters stay in place but are constantly changing
  empty
} screenCharType;

typedef struct screenChar
{
  screenCharType type;
  int framesLeft;
  char index;
} screenChar;

typedef struct charSpawner
{
  bool spawning;
  int time;
} charSpawner;

// Main
int main(int argc, char* argv[])
{
  // Set the locale
  setlocale(LC_ALL, "");

  // Initialize the window
  WINDOW *win;
  if ((win = initscr()) == NULL || !has_colors())
  {
    exit(EXIT_FAILURE);
  }

  // Set the options
  raw();
  noecho();
  curs_set(0);
  nodelay(win, TRUE);
  scrollok(win, TRUE);
  start_color();
  init_pair(1, COLOR_WHITE, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);

  // Seed the random
  srand(time(NULL));

  // Get the screen size
  int w = 0, h = 0;
  getmaxyx(win, h, w);

  // Halve the width
  w = w / 2;

  // Initialize the character list
  wchar_t alphabet[] = {L"゠ァアィイゥウェエォオカガキギクグケゲコゴサザシジスズセゼソゾタ"
                         "ダチヂッツヅテデトドナニヌネノハバパヒビピフブプヘベペホボポマミ"
                         "ムメモャヤュユョヨラリルレロヮワヰヱヲンヴヵヶヷヸヹヺ・ーヽヾヿ"};

  int alphabetSize = 96;

  // Initialize the screen character map
  screenChar *buffer = malloc(w * h * sizeof(screenChar));
  for (int i = 0; i < w * h; i++)
  {
    buffer[i].type = empty;
  }

  // Initialize the spawners
  charSpawner *spawners = malloc(w * sizeof(charSpawner));
  for (int i = 0; i < w; i++)
  {
    if (rand() % 150 == 1)
    {
      spawners[i].spawning = true;
      spawners[i].time = (rand() % 15) + 5;
    }
    else
    {
      spawners[i].spawning = false;
      spawners[i].time += rand() % 300;
    }
  }

  // Start the loop
  struct timespec t0, tf;
  while(getch() == -1)
  {
    // Get the time at the start of the loop
    clock_gettime(CLOCK_REALTIME, &t0);

    // Update all the spawners
    for (int i = 0; i < w; i++)
    {
      // If this spawner is active
      if (spawners[i].spawning)
      {
        // Spawn a spawner character
        buffer[i].type = spawner;
        buffer[i].index = rand() % alphabetSize;
        // Set the lifetime
        buffer[i].framesLeft = spawners[i].time;
        // Update the time until the next switch
        spawners[i].time += rand() % 150 + 150;
        // Set it to not spawn again
        spawners[i].spawning = false;
      }
      // If the spawner time is up
      if (spawners[i].time-- == 0)
      {
        // Set it to spawn on the next loop
        spawners[i].spawning = true;
        // This is actually the size of the falling "line"
        spawners[i].time = (rand() % 25) + 5;
      }
    }

    // Update all the characters
    clear();
    for (int x = w - 1; x >= 0; x--)
    {
      for (int y = h - 1; y >= 0; y--)
      {
        screenChar *currentChar = &buffer[y * w + x];
        switch(currentChar->type)
        {
          case spawner:
            attron(COLOR_PAIR(1));
            mvprintw(y, x*2, "%lc", alphabet[currentChar->index]);

            // "Move" the spawner one character down
            if (y < (h - 1))
            {
              screenChar *nextChar = &buffer[(y + 1) * w + x];
              nextChar->type = spawner;
              nextChar->framesLeft = currentChar->framesLeft;
              nextChar->index = rand() % alphabetSize;
            }

            // There's a small chance that the character will get stuck
            if (rand() % 80)
            {
              currentChar->type = stuckStatic;
            }
            else
            {
              currentChar->type = stuckChanging;
              //currentChar->framesLeft += rand() % 5;
            }

            break;
          case stuckStatic:
            attron(COLOR_PAIR(2));
            mvprintw(y, x*2, "%lc", alphabet[currentChar->index]);

            if (currentChar->framesLeft-- <= 0)
            {
              currentChar->type = empty;
            }
            else if (rand() % 220 == 0)
            {
              currentChar->type = stuckChanging;
              currentChar->framesLeft += rand() % 10 + 5;
            }
            break;
          case stuckChanging:
            if (currentChar->framesLeft % 2)
            {
              currentChar->index = rand() % alphabetSize;
            }
            attron(COLOR_PAIR(1));
            mvprintw(y, x*2, "%lc", alphabet[currentChar->index]);
            if (currentChar->framesLeft-- <= 0)
            {
              currentChar->type = empty;
            }
            break;
          case empty:
            break;
        }
      }
    }

    // Refresh the screen
    refresh();

    // Ensure we have a reasonable framerate
    clock_gettime(CLOCK_REALTIME, &tf);
    usleep(1000000 / 10 - (tf.tv_nsec - t0.tv_nsec) / 1000);
  }

  //
  endwin();
  return EXIT_SUCCESS;
}
