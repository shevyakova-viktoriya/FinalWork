#include <stdio.h>
#include <stdlib.h>
#include <time.h>    // Для srand()
#include <unistd.h>  // Для usleep()
#include <termios.h> // Для неблокирующего ввода
#include <fcntl.h>   // Для fcntl()

// Размеры игрового поля
#define WIDTH 40
#define HEIGHT 20

// Символы для отрисовки
#define SNAKE_HEAD_CHAR '@'
#define SNAKE_BODY_CHAR 'o'
#define FOOD_CHAR '*'
#define DECREASE_CHAR '-' // Символ для уменьшения змейки
#define WALL_CHAR '#'

// Направления движения
enum Direction {
    STOP = 0,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

enum Direction dir;

// Координаты змейки
typedef struct {
    int x;
    int y;
} Segment;

Segment snake[100]; // Максимальная длина змейки
int snakeLength;

// Координаты еды
int foodX, foodY;
int decreaseFoodX, decreaseFoodY; // Координаты для уменьшающей еды

int score; // Очки
int gameOver; // Флаг окончания игры

// Цветовые настройки (ANSI escape codes)
// https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_WHITE   "\x1b[37m"

char* snakeColor = ANSI_COLOR_GREEN;
char* foodColor = ANSI_COLOR_RED;
char* decreaseFoodColor = ANSI_COLOR_BLUE;
char* wallColor = ANSI_COLOR_WHITE;
char* textColor = ANSI_COLOR_WHITE;

// Для сохранения оригинальных настроек терминала
static struct termios old_tio, new_tio;

// Функция для установки цвета текста в консоли (Unix-like specific)
void setConsoleColor(const char* color) {
    printf("%s", color);
}

// Установка курсора в заданную позицию (Unix-like specific)
void gotoxy(int x, int y) {
    printf("\x1b[%d;%dH", y + 1, x + 1); // ANSI escape code для позиционирования курсора
}

// Очистка экрана (Unix-like specific)
void clearScreen() {
    printf("\x1b[2J"); // ANSI escape code для очистки экрана
}

// Настройка терминала для неблокирующего ввода
void set_terminal_raw_mode() {
    tcgetattr(STDIN_FILENO, &old_tio); // Сохранить текущие настройки
    new_tio = old_tio;
    new_tio.c_lflag &= (~ICANON & ~ECHO); // Отключить канонический режим и эхо
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio); // Применить новые настройки
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK); // Неблокирующий ввод
}

// Восстановление настроек терминала
void reset_terminal_mode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio); // Восстановить старые настройки
}

// Проверка наличия нажатой клавиши (Unix-like specific)
int kbhit() {
    char buf[1];
    int rd;
    rd = read(STDIN_FILENO, buf, 1);
    if (rd > 0) {
        ungetc(buf[0], stdin); // Вернуть символ обратно в буфер
        return 1;
    }
    return 0;
}

// Чтение нажатой клавиши (Unix-like specific)
int getch_nonblock() {
    int c = getchar();
    if (c == EOF) {
        return 0; // Возвращаем 0, если ничего не было прочитано
    }
    return c;
}


// Инициализация игры
void Setup() {
    gameOver = 0;
    dir = STOP;
    snakeLength = 1;
    snake[0].x = WIDTH / 2;
    snake[0].y = HEIGHT / 2;
    score = 0;

    // Генерация первой еды
    srand(time(NULL));
    foodX = rand() % (WIDTH - 2) + 1; // -2 и +1 для того, чтобы еда не появлялась на стенах
    foodY = rand() % (HEIGHT - 2) + 1;

    // Генерация уменьшающей еды (редко)
    decreaseFoodX = -1; // Изначально скрыта
    decreaseFoodY = -1;
    if (rand() % 5 == 0) { // Шанс 1/5 что появится уменьшающая еда
        decreaseFoodX = rand() % (WIDTH - 2) + 1;
        decreaseFoodY = rand() % (HEIGHT - 2) + 1;
    }
}

// Отрисовка игрового поля
void Draw() {
    clearScreen(); // Очистка экрана
    gotoxy(0, 0);  // Установка курсора в верхний левый угол

    setConsoleColor(wallColor);
    // Верхняя граница
    for (int i = 0; i < WIDTH; i++) {
        printf("%c", WALL_CHAR);
    }
    printf("\n");

    for (int i = 0; i < HEIGHT - 2; i++) { // HEIGHT - 2, так как 2 линии заняты границами
        for (int j = 0; j < WIDTH; j++) {
            if (j == 0 || j == WIDTH - 1) { // Боковые границы
                setConsoleColor(wallColor);
                printf("%c", WALL_CHAR);
            } else {
                int isBody = 0;
                // Проверяем, не является ли текущая позиция головой или телом змейки
                if (i == snake[0].y - 1 && j == snake[0].x) { // Голова змейки
                    setConsoleColor(snakeColor);
                    printf("%c", SNAKE_HEAD_CHAR);
                } else if (i == foodY - 1 && j == foodX) { // Еда
                    setConsoleColor(foodColor);
                    printf("%c", FOOD_CHAR);
                } else if (i == decreaseFoodY - 1 && j == decreaseFoodX) { // Уменьшающая еда
                    setConsoleColor(decreaseFoodColor);
                    printf("%c", DECREASE_CHAR);
                } else {
                    for (int k = 1; k < snakeLength; k++) {
                        if (i == snake[k].y - 1 && j == snake[k].x) {
                            setConsoleColor(snakeColor);
                            printf("%c", SNAKE_BODY_CHAR);
                            isBody = 1;
                            break;
                        }
                    }
                    if (!isBody) {
                        setConsoleColor(textColor); // Обычный цвет для пустого пространства
                        printf(" ");
                    }
                }
            }
        }
        printf("\n");
    }

    setConsoleColor(wallColor);
    // Нижняя граница
    for (int i = 0; i < WIDTH; i++) {
        printf("%c", WALL_CHAR);
    }
    printf("\n");

    setConsoleColor(textColor);
    printf("Score: %d\n", score);
    printf("Press 'C' to change snake color, 'F' for food, 'W' for wall, 'T' for text, 'E' for decrease food color, 'K' for wall color.\n");
    printf("Press 'Q' to quit.\n");
    setConsoleColor(ANSI_COLOR_RESET); // Сброс цвета после отрисовки
}

// Обработка ввода
void Input() {
    if (kbhit()) {
        int key = getch_nonblock();
        switch (key) {
            case 'a':
            case 'A':
                if (dir != RIGHT) dir = LEFT;
                break;
            case 'd':
            case 'D':
                if (dir != LEFT) dir = RIGHT;
                break;
            case 'w':
            case 'W':
                if (dir != DOWN) dir = UP;
                break;
            case 's':
            case 'S':
                if (dir != UP) dir = DOWN;
                break;
            case 'q':
            case 'Q':
                gameOver = 1;
                break;
            case 'c': // Изменить цвет змейки
            case 'C':
                snakeColor = (snakeColor == ANSI_COLOR_GREEN) ? ANSI_COLOR_CYAN : ANSI_COLOR_GREEN;
                break;
            case 'f': // Изменить цвет еды
            case 'F':
                foodColor = (foodColor == ANSI_COLOR_RED) ? ANSI_COLOR_YELLOW : ANSI_COLOR_RED;
                break;
            case 't': // Изменить цвет текста (фона)
            case 'T':
                textColor = (textColor == ANSI_COLOR_WHITE) ? ANSI_COLOR_MAGENTA : ANSI_COLOR_WHITE;
                break;
            case 'e': // Изменить цвет уменьшающей еды
            case 'E':
                decreaseFoodColor = (decreaseFoodColor == ANSI_COLOR_BLUE) ? ANSI_COLOR_MAGENTA : ANSI_COLOR_BLUE;
                break;
            case 'k': // Изменить цвет стен
            case 'K':
                wallColor = (wallColor == ANSI_COLOR_WHITE) ? ANSI_COLOR_RED : ANSI_COLOR_WHITE;
                break;
        }
    }
}

// Логика игры
void Logic() {
    // Передвижение тела змейки
    for (int i = snakeLength - 1; i > 0; i--) {
        snake[i] = snake[i - 1];
    }

    // Передвижение головы
    switch (dir) {
        case LEFT:
            snake[0].x--;
            break;
        case RIGHT:
            snake[0].x++;
            break;
        case UP:
            snake[0].y--;
            break;
        case DOWN:
            snake[0].y++;
            break;
        default:
            break;
    }

    // Проверка столкновений со стенами
    if (snake[0].x < 1 || snake[0].x >= WIDTH - 1 || snake[0].y < 1 || snake[0].y >= HEIGHT - 1) {
        gameOver = 1;
    }

    // Проверка столкновений с собой
    for (int i = 1; i < snakeLength; i++) {
        if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
            gameOver = 1;
        }
    }

    // Поедание еды
    if (snake[0].x == foodX && snake[0].y == foodY) {
        score += 10;
        snakeLength++;

        // Генерация новой еды
        foodX = rand() % (WIDTH - 2) + 1;
        foodY = rand() % (HEIGHT - 2) + 1;

        // Шанс на появление уменьшающей еды
        if (rand() % 3 == 0) { // 1/3 шанс
            decreaseFoodX = rand() % (WIDTH - 2) + 1;
            decreaseFoodY = rand() % (HEIGHT - 2) + 1;
        } else {
            decreaseFoodX = -1; // Скрыть
            decreaseFoodY = -1;
        }
    }

    // Поедание уменьшающей еды
    if (snake[0].x == decreaseFoodX && snake[0].y == decreaseFoodY && decreaseFoodX != -1) {
        score -= 5; // Штраф
        if (snakeLength > 1) { // Змейка не может быть короче 1 сегмента
            snakeLength--;
        }
        decreaseFoodX = -1; // Убрать
        decreaseFoodY = -1;
    }
}

int main() {
    set_terminal_raw_mode(); // Установить терминал в raw mode
    Setup();
    while (!gameOver) {
        Draw();
        Input();
        Logic();
        usleep(100000); // Задержка для скорости игры (в микросекундах)
    }

    clearScreen(); // Очистить экран после завершения игры
    gotoxy(0, 0);  // Вернуть курсор в начало
    setConsoleColor(textColor); // Восстановить стандартный цвет
    printf("\nGame Over!\nFinal Score: %d\n", score);
    setConsoleColor(ANSI_COLOR_RESET); // Сброс цвета
    getchar(); // Ждать нажатия любой клавиши перед выходом
    reset_terminal_mode(); // Восстановить настройки терминала
    return 0;
}