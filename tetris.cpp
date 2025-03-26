#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <termios.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <fstream>
#include <cstring> // Include this header for memcpy
using namespace std;

const int consoleWidth = 50; // Console Screen Size X (columns)
const int consoleHeight = 40; // Console Screen Size Y (rows)
const int fieldWidth = 20; // Field Width
const int fieldHeight = 30; // Field Height

enum GameMode { DEFAULT, RANDOM };

class TetrisGame 
{
public:
    TetrisGame(GameMode gameMode) : gameMode(gameMode), currentPiece(0), currentRotation(0), currentX(fieldWidth / 2), currentY(0), speed(20), speedCounter(0), forcePieceDown(false), rotationHold(true), pieceCounter(0), score(0), isGameOver(false), isPaused(false), level(1), highScore(0), nextPiece(rand() % 7), previousField(nullptr), previousPiece(-1), previousRotation(0), previousX(0), previousY(0), previousScore(0) 
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
        delete[] previousField;
        saveHighScore();
    }

    void run() 
    {
        showStartingAnimation();
        setTerminalRawMode(true);
        while (!isGameOver) 
        {
            if (!isPaused) 
            {
                this_thread::sleep_for(chrono::milliseconds(50)); // Small Step = 1 Game Tick
                speedCounter++;
                forcePieceDown = (speedCounter == speed);

                handleInput();
                updateGame();
                drawGame();

                // Reset input keys
                fill(begin(keys), end(keys), false);
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
    GameMode gameMode;
    wstring tetrominoes[7];
    unsigned char *field;
    wchar_t *screen;
    bool keys[4] = {false, false, false, false};
    int currentPiece;
    int currentRotation;
    int currentX;
    int currentY;
    int speed;
    int speedCounter;
    bool forcePieceDown;
    bool rotationHold;
    int pieceCounter;
    int score;
    vector<int> completedLines;
    bool isGameOver;
    bool isPaused;
    int level;
    int highScore;
    int nextPiece;

    // Variables for undo functionality
    unsigned char *previousField;
    int previousPiece;
    int previousRotation;
    int previousX;
    int previousY;
    int previousScore;

    void initializeField() 
    {
        field = new unsigned char[fieldWidth * fieldHeight];
        for (int x = 0; x < fieldWidth; x++)
            for (int y = 0; y < fieldHeight; y++)
                field[y * fieldWidth + x] = (x == 0 || x == fieldWidth - 1 || y == fieldHeight - 1) ? 9 : 0;
    }

    void initializePieces() 
    {
        tetrominoes[0].append(L"..X...X...X...X."); // Tetronimos 4x4
        tetrominoes[1].append(L"..X..XX...X.....");
        tetrominoes[2].append(L".....XX..XX.....");
        tetrominoes[3].append(L"..X..XX..X......");
        tetrominoes[4].append(L".X...XX...X.....");
        tetrominoes[5].append(L".X...X...XX.....");
        tetrominoes[6].append(L"..X...X..XX.....");
    }

    void initializeScreen() 
    {
        screen = new wchar_t[consoleWidth * consoleHeight];
        for (int i = 0; i < consoleWidth * consoleHeight; i++) screen[i] = L' ';
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
                int fi = (posY + py) * fieldWidth + (posX + px);
                if (posX + px >= 0 && posX + px < fieldWidth) 
                {
                    if (posY + py >= 0 && posY + py < fieldHeight) 
                    {
                        if (tetrominoes[pieceIdx][pi] != L'.' && field[fi] != 0)
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
                case 'd': keys[0] = true; break; // Move right
                case 'a': keys[1] = true; break; // Move left
                case 's': keys[2] = true; break; // Move down
                case 'w': keys[3] = true; break; // Rotate
                case 'x': isGameOver = true; break; // Exit
                case 'r': initialize(); break; // Restart
                case 'p': isPaused = !isPaused; break; // Pause
                case ' ': dropPiece(); break; // Drop piece instantly
                case 'u': undo(); break; // Undo last piece
                default: break;
            }
        }
    }

    void dropPiece() 
    {
        while (doesPieceFit(currentPiece, currentRotation, currentX, currentY + 1)) 
        {
            currentY++;
        }
        forcePieceDown = true;
    }

    void initialize() 
    {
        currentPiece = 0;
        currentRotation = 0;
        currentX = fieldWidth / 2;
        currentY = 0;
        speed = 20;
        speedCounter = 0;
        forcePieceDown = false;
        rotationHold = true;
        pieceCounter = 0;
        score = 0;
        completedLines.clear();
        isGameOver = false;
        isPaused = false;
        level = 1;
        nextPiece = rand() % 7;
        initializeField();
        delete[] previousField;
        previousField = nullptr;
    }

    void updateGame() 
    {
        currentX += (keys[0] && doesPieceFit(currentPiece, currentRotation, currentX + 1, currentY)) ? 1 : 0;
        currentX -= (keys[1] && doesPieceFit(currentPiece, currentRotation, currentX - 1, currentY)) ? 1 : 0;
        currentY += (keys[2] && doesPieceFit(currentPiece, currentRotation, currentX, currentY + 1)) ? 1 : 0;

        if (keys[3]) 
        {
            currentRotation += (rotationHold && doesPieceFit(currentPiece, currentRotation + 1, currentX, currentY)) ? 1 : 0;
            rotationHold = false;
        } 
        else
            rotationHold = true;

        if (forcePieceDown) 
        {
            speedCounter = 0;
            pieceCounter++;
            if (pieceCounter % 50 == 0)
                if (speed >= 10) speed--;

            if (gameMode == RANDOM) // 100% chance to change piece
            {
                currentPiece = rand() % 7;
            }

            if (doesPieceFit(currentPiece, currentRotation, currentX, currentY + 1))
                currentY++;
            else 
            {
                // Save the current state before placing the piece
                saveState();

                for (int px = 0; px < 4; px++)
                    for (int py = 0; py < 4; py++)
                        if (tetrominoes[currentPiece][rotate(px, py, currentRotation)] != L'.')
                            field[(currentY + py) * fieldWidth + (currentX + px)] = currentPiece + 1;

                for (int py = 0; py < 4; py++)
                    if (currentY + py < fieldHeight - 1) 
                    {
                        bool line = true;
                        for (int px = 1; px < fieldWidth - 1; px++)
                            line &= (field[(currentY + py) * fieldWidth + px]) != 0;

                        if (line) 
                        {
                            for (int px = 1; px < fieldWidth - 1; px++)
                                field[(currentY + py) * fieldWidth + px] = 8;
                            completedLines.push_back(currentY + py);
                        }
                    }

                score += 25;
                if (!completedLines.empty()) 
                score += (1 << completedLines.size()) * 100;

                currentPiece = nextPiece;
                nextPiece = rand() % 7;

                currentX = fieldWidth / 2;
                currentY = 0;
                currentRotation = 0;

                isGameOver = !doesPieceFit(currentPiece, currentRotation, currentX, currentY);

                // Increase level and speed
                if (score / 1000 > level - 1) 
                {
                    level++;
                    speed = max(10, speed - 2);
                }
            }
        }
    }

    void drawGame() 
    {
        system("clear");

        for (int x = 0; x < fieldWidth; x++)
            for (int y = 0; y < fieldHeight; y++)
                screen[(y + 2) * consoleWidth + (x + 2)] = L" ABCDEFG=#"[field[y * fieldWidth + x]];

        for (int px = 0; px < 4; px++)
            for (int py = 0; py < 4; py++)
                if (tetrominoes[currentPiece][rotate(px, py, currentRotation)] != L'.')
                    screen[(currentY + py + 2) * consoleWidth + (currentX + px + 2)] = currentPiece + 65;

        // Draw the next piece
        drawNextPiece();

        // Display the game screen and additional information
        for (int i = 0; i < consoleHeight; i++) {
            for (int j = 0; j < consoleWidth; j++) 
            {
                wcout << screen[i * consoleWidth + j];
            }

            // Display instructions, live score, level, and high score next to the map
            if (i == 2) wcout << L" Use W/A/S/D to move.";
            if (i == 3) wcout << L" Press X to quit.";
            if (i == 4) wcout << L" Press R to restart.";
            if (i == 5) wcout << L" Press P to pause.";
            if (i == 6) wcout << L" Press Space to drop.";
            if (i == 7) wcout << L" Press U to undo.";
            if (i == 8) wcout << L" Live Score: " << score;
            if (i == 9) wcout << L" Level: " << level;
            if (i == 10) wcout << L" High Score: " << highScore;
            if (i == 11) wcout << L" 25 pts when a piece is placed,";
            if (i == 12) wcout << L" 100 pts for each line cleared";

            wcout << endl;
        }

        if (!completedLines.empty()) 
        {
            this_thread::sleep_for(chrono::milliseconds(400));

            for (auto &line : completedLines)
                for (int px = 1; px < fieldWidth - 1; px++)
                {
                    for (int py = line; py > 0; py--)
                        field[py * fieldWidth + px] = field[(py - 1) * fieldWidth + px];
                    field[px] = 0;
                }

            completedLines.clear();
        }
    }

    void drawNextPiece() 
    {
        int offsetX = fieldWidth + 5; // Position to the right of the field
        int offsetY = 5; // Position below the top of the screen

        // Clear the area where the next piece will be drawn
        for (int px = 0; px < 4; px++)
            for (int py = 0; py < 4; py++)
                screen[(offsetY + py) * consoleWidth + (offsetX + px)] = L' ';

        // Draw the next piece
        for (int px = 0; px < 4; px++)
            for (int py = 0; py < 4; py++)
                if (tetrominoes[nextPiece][rotate(px, py, 0)] != L'.')
                    screen[(offsetY + py) * consoleWidth + (offsetX + px)] = nextPiece + 65;
    }

    void saveState() 
    {
        delete[] previousField;
        previousField = new unsigned char[fieldWidth * fieldHeight];
        memcpy(previousField, field, fieldWidth * fieldHeight * sizeof(unsigned char));
        previousPiece = currentPiece;
        previousRotation = currentRotation;
        previousX = currentX;
        previousY = currentY;
        previousScore = score;
    }

    void undo() 
    {
        if (previousField) 
        {
            // Clear the last placed piece from the field
            for (int px = 0; px < 4; px++)
                for (int py = 0; py < 4; py++)
                    if (tetrominoes[previousPiece][rotate(px, py, previousRotation)] != L'.')
                        field[(previousY + py) * fieldWidth + (previousX + px)] = 0;

            // Restore the previous state
            memcpy(field, previousField, fieldWidth * fieldHeight * sizeof(unsigned char));
            currentPiece = previousPiece;
            currentRotation = previousRotation;
            currentX = fieldWidth / 2; // Reset X position to the top
            currentY = 0; // Reset Y position to the top
            score = previousScore;
            delete[] previousField;
            previousField = nullptr;
        }
    }
};

int main() 
{
    int modeChoice;
    cout << "Select Game Mode: 1. Default 2. Random Pieces" << endl;
    cin >> modeChoice;

    GameMode gameMode = (modeChoice == 2) ? RANDOM : DEFAULT;

    TetrisGame game(gameMode);
    game.run();
    return 0;
}
