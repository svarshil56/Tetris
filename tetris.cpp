#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <termios.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <fstream>
using namespace std;

const int scrWidth = 50; // Console Screen Size X (columns)
const int scrHeight = 40; // Console Screen Size Y (rows)
const int fldWidth = 20; // Field Width
const int fldHeight = 30; // Field Height

enum GameMode { DEFAULT, RANDOM };

class TetrisGame 
{
public:
    TetrisGame(GameMode mode) : mode(mode), curPiece(0), curRot(0), curX(fldWidth / 2), curY(0), spd(20), spdCount(0), forceDown(false), rotHold(true), pieceCount(0), score(0), gameOver(false), paused(false), level(1), highScore(0) 
    {
        srand(time(0));
        initializeField();
        initializePieces();
        initializeScreen();
        loadHighScore();
    }

    ~TetrisGame() 
    {
        delete[] field;
        delete[] screen;
        saveHighScore();
    }

    void run() 
    {
        showStartingAnimation();
        setTerminalRawMode(true);
        while (!gameOver) 
        {
            if (!paused) 
            {
                this_thread::sleep_for(chrono::milliseconds(50)); // Small Step = 1 Game Tick
                spdCount++;
                forceDown = (spdCount == spd);

                handleInput();
                updateGame();
                drawGame();

                // Reset input keys
                fill(begin(key), end(key), false);
            } 
            else 
            {
                handleInput();
            }
        }
        setTerminalRawMode(false);
        cout << "Game Over!! Score:" << score << endl;
    }

private:
    GameMode mode;
    wstring pieces[7];
    unsigned char *field;
    wchar_t *screen;
    bool key[4] = {false, false, false, false};
    int curPiece;
    int curRot;
    int curX;
    int curY;
    int spd;
    int spdCount;
    bool forceDown;
    bool rotHold;
    int pieceCount;
    int score;
    vector<int> lines;
    bool gameOver;
    bool paused;
    int level;
    int highScore;

    void initializeField() 
    {
        field = new unsigned char[fldWidth * fldHeight];
        for (int x = 0; x < fldWidth; x++)
            for (int y = 0; y < fldHeight; y++)
                field[y * fldWidth + x] = (x == 0 || x == fldWidth - 1 || y == fldHeight - 1) ? 9 : 0;
    }

    void initializePieces() 
    {
        pieces[0].append(L"..X...X...X...X."); // Tetronimos 4x4
        pieces[1].append(L"..X..XX...X.....");
        pieces[2].append(L".....XX..XX.....");
        pieces[3].append(L"..X..XX..X......");
        pieces[4].append(L".X...XX...X.....");
        pieces[5].append(L".X...X...XX.....");
        pieces[6].append(L"..X...X..XX.....");
    }

    void initializeScreen() 
    {
        screen = new wchar_t[scrWidth * scrHeight];
        for (int i = 0; i < scrWidth * scrHeight; i++) screen[i] = L' ';
    }

    void loadHighScore() 
    {
        ifstream file("highscore.txt");
        if (file.is_open()) 
        {
            file >> highScore;
            file.close();
        }
    }

    void saveHighScore() 
    {
        if (score > highScore) 
        {
            highScore = score;
            ofstream file("highscore.txt");
            if (file.is_open()) 
            {
                file << highScore;
                file.close();
            }
        }
    }

    void showStartingAnimation() 
    {
        system("clear");
        string text = "TETRIS GAME";
        string displayText = "           ";
        for (int i = 0; i < 11; ++i) 
        {
            displayText[i] = text[i];
            system("clear");
            for (int j = 0; j < 10; ++j) 
            {
                if (j == 2) 
                {
                    cout << displayText;
                } 
                else 
                {
                    cout << "     ";
                }
                cout << endl;
            }
            usleep(200000);
        }
    }

    int rotate(int px, int py, int r) 
    {
        int pi = 0;
        switch (r % 4) 
        {
            case 0: pi = py * 4 + px; break; // 0 degrees
            case 1: pi = 12 + py - (px * 4); break; // 90 degrees
            case 2: pi = 15 - (py * 4) - px; break; // 180 degrees
            case 3: pi = 3 - py + (px * 4); break; // 270 degrees
        }
        return pi;
    }

    bool doesPieceFit(int pieceIdx, int rot, int posX, int posY) 
    {
        for (int px = 0; px < 4; px++)
            for (int py = 0; py < 4; py++) 
            {
                int pi = rotate(px, py, rot);
                int fi = (posY + py) * fldWidth + (posX + px);
                if (posX + px >= 0 && posX + px < fldWidth) 
                {
                    if (posY + py >= 0 && posY + py < fldHeight) 
                    {
                        if (pieces[pieceIdx][pi] != L'.' && field[fi] != 0)
                            return false;
                    }
                }
            }
        return true;
    }

    bool kbhit() 
    {
        struct timeval tv = {0L, 0L};
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    }

    char getch() 
    {
        char buf = 0;
        if (read(STDIN_FILENO, &buf, 1) < 0) 
        {
            perror("read()");
        }
        return buf;
    }

    void setTerminalRawMode(bool enable) 
    {
        static struct termios oldt, newt;
        if (enable) 
        {
            tcgetattr(STDIN_FILENO, &oldt);
            newt = oldt;
            newt.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        } 
        else
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        }
    }

    void handleInput() 
    {
        if (kbhit()) 
        {
            char keyPressed = getch();
            switch (keyPressed) 
            {
                case 'd': key[0] = true; break; // Move right
                case 'a': key[1] = true; break; // Move left
                case 's': key[2] = true; break; // Move down
                case 'w': key[3] = true; break; // Rotate
                case 'x': gameOver = true; break; // Exit
                case 'r': initialize(); break; // Restart
                case 'p': paused = !paused; break; // Pause
                default: break;
            }
        }
    }

    void initialize() 
    {
        curPiece = 0;
        curRot = 0;
        curX = fldWidth / 2;
        curY = 0;
        spd = 20;
        spdCount = 0;
        forceDown = false;
        rotHold = true;
        pieceCount = 0;
        score = 0;
        lines.clear();
        gameOver = false;
        paused = false;
        level = 1;
        initializeField();
    }

    void updateGame() 
    {
        curX += (key[0] && doesPieceFit(curPiece, curRot, curX + 1, curY)) ? 1 : 0;
        curX -= (key[1] && doesPieceFit(curPiece, curRot, curX - 1, curY)) ? 1 : 0;
        curY += (key[2] && doesPieceFit(curPiece, curRot, curX, curY + 1)) ? 1 : 0;

        if (key[3]) 
        {
            curRot += (rotHold && doesPieceFit(curPiece, curRot + 1, curX, curY)) ? 1 : 0;
            rotHold = false;
        } 
        else
            rotHold = true;

        if (forceDown) 
        {
            spdCount = 0;
            pieceCount++;
            if (pieceCount % 50 == 0)
                if (spd >= 10) spd--;

            if (mode == RANDOM) // 100% chance to change piece
            {
                curPiece = rand() % 7;
            }

            if (doesPieceFit(curPiece, curRot, curX, curY + 1))
                curY++;
            else 
            {
                for (int px = 0; px < 4; px++)
                    for (int py = 0; py < 4; py++)
                        if (pieces[curPiece][rotate(px, py, curRot)] != L'.')
                            field[(curY + py) * fldWidth + (curX + px)] = curPiece + 1;

                for (int py = 0; py < 4; py++)
                    if (curY + py < fldHeight - 1) 
                    {
                        bool line = true;
                        for (int px = 1; px < fldWidth - 1; px++)
                            line &= (field[(curY + py) * fldWidth + px]) != 0;

                        if (line) 
                        {
                            for (int px = 1; px < fldWidth - 1; px++)
                                field[(curY + py) * fldWidth + px] = 8;
                            lines.push_back(curY + py);
                        }
                    }

                score += 25;
                if (!lines.empty()) 
                score += (1 << lines.size()) * 100;

                curX = fldWidth / 2;
                curY = 0;
                curRot = 0;
                curPiece = rand() % 7;

                gameOver = !doesPieceFit(curPiece, curRot, curX, curY);

                // Increase level and speed
                if (score / 1000 > level - 1) 
                {
                    level++;
                    spd = max(10, spd - 2);
                }
            }
        }
    }

    void drawGame() 
    {
        system("clear");

        for (int x = 0; x < fldWidth; x++)
            for (int y = 0; y < fldHeight; y++)
                screen[(y + 2) * scrWidth + (x + 2)] = L" ABCDEFG=#"[field[y * fldWidth + x]];

        for (int px = 0; px < 4; px++)
            for (int py = 0; py < 4; py++)
                if (pieces[curPiece][rotate(px, py, curRot)] != L'.')
                    screen[(curY + py + 2) * scrWidth + (curX + px + 2)] = curPiece + 65;

        for (int i = 0; i < scrHeight; i++) {
            for (int j = 0; j < scrWidth; j++) 
            {
                wcout << screen[i * scrWidth + j];
            }
            wcout << endl;
        }

        if (!lines.empty()) 
        {
            this_thread::sleep_for(chrono::milliseconds(400));

            for (auto &v : lines)
                for (int px = 1; px < fldWidth - 1; px++)
                {
                    for (int py = v; py > 0; py--)
                        field[py * fldWidth + px] = field[(py - 1) * fldWidth + px];
                    field[px] = 0;
                }

            lines.clear();
        }

        // Display instructions, live score, level, and high score
        wcout << L"Use W/A/S/D to move. Press X to quit. Press R to restart. Press P to pause." << endl;
        wcout << L"Live Score: " << score << endl;
        wcout << L"Level: " << level << endl;
        wcout << L"High Score: " << highScore << endl;
        wcout << L"25 pts when a piece is placed, 100 pts for each line cleared" << endl;
    }
};

int main() 
{
    int modeChoice;
    cout << "Select Game Mode: 1. Default 2. Random Pieces" << endl;
    cin >> modeChoice;

    GameMode mode = (modeChoice == 2) ? RANDOM : DEFAULT;

    TetrisGame game(mode);
    game.run();
    return 0;
}