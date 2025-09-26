#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

// Максимальный размер змейки
#define MAX_SNAKE_LENGTH 100

// Координаты
typedef struct {
    int x;
    int y;
} Point;

// Структура для змейки
typedef struct {
    Point body[MAX_SNAKE_LENGTH];
    int length;
    int direction; // 0: право, 1: вниз, 2: лево, 3: верх
} Snake;

// Глобальные переменные
Snake snake;
Point food;
int score;
int game_over;
int delay_ms; // Задержка для скорости игры
int grow_shrink_counter; // Счетчик для роста/уменьшения
int color_scheme; // Цветовая палитра

// Прототипы функций
void init_game();
void draw_game();
void generate_food();
void handle_input();
void update_game();
void end_game();
void change_color_scheme();

int main() {
    initscr();              // Инициализация ncurses
    cbreak();               // Немедленный ввод символов
    noecho();               // Не отображать введенные символы
    keypad(stdscr, TRUE);   // Включение специальных клавиш (стрелки)
    curs_set(0);            // Скрыть курсор
    timeout(0);             // Неблокирующий ввод

    if (has_colors()) {
        start_color();
        // Инициализация цветовых пар
        init_pair(1, COLOR_WHITE, COLOR_BLACK); // Основная схема
        init_pair(2, COLOR_GREEN, COLOR_BLACK); // Змейка (зеленая)
        init_pair(3, COLOR_RED, COLOR_BLACK);   // Еда (красная)

        init_pair(4, COLOR_BLUE, COLOR_BLACK);  // Схема 2: Змейка (синяя)
        init_pair(5, COLOR_YELLOW, COLOR_BLACK);// Схема 2: Еда (желтая)
        
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK); // Схема 3: Змейка (пурпурная)
        init_pair(7, COLOR_CYAN, COLOR_BLACK);    // Схема 3: Еда (голубая)
    }

    srand(time(NULL)); // Инициализация генератора случайных чисел

    init_game();

    while (!game_over) {
        handle_input();
        update_game();
        draw_game();
        usleep(delay_ms * 1000); // Задержка в микросекундах
    }

    end_game();
    return 0;
}

void init_game() {
    // Инициализация змейки
    snake.length = 3;
    snake.body[0].x = COLS / 2;
    snake.body[0].y = LINES / 2;
    snake.body[1].x = COLS / 2 - 1;
    snake.body[1].y = LINES / 2;
    snake.body[2].x = COLS / 2 - 2;
    snake.body[2].y = LINES / 2;
    snake.direction = 0; // Изначально движется вправо

    score = 0;
    game_over = 0;
    delay_ms = 200; // Начальная скорость
    grow_shrink_counter = 0;
    color_scheme = 0; // Изначально первая цветовая схема

    generate_food();
}

void draw_game() {
    clear(); // Очистить экран

    // Установка цветовой палитры
    int snake_color_pair;
    int food_color_pair;
    switch (color_scheme) {
        case 0: // Схема 1 (Зеленая змейка, Красная еда)
            snake_color_pair = 2;
            food_color_pair = 3;
            break;
        case 1: // Схема 2 (Синяя змейка, Желтая еда)
            snake_color_pair = 4;
            food_color_pair = 5;
            break;
        case 2: // Схема 3 (Пурпурная змейка, Голубая еда)
            snake_color_pair = 6;
            food_color_pair = 7;
            break;
        default:
            snake_color_pair = 2;
            food_color_pair = 3;
            break;
    }

    // Отрисовка еды
    attron(COLOR_PAIR(food_color_pair));
    mvprintw(food.y, food.x, "*");
    attroff(COLOR_PAIR(food_color_pair));

    // Отрисовка змейки
    attron(COLOR_PAIR(snake_color_pair));
    for (int i = 0; i < snake.length; i++) {
        mvprintw(snake.body[i].y, snake.body[i].x, "o");
    }
    attroff(COLOR_PAIR(snake_color_pair));

    // Отрисовка границ (по желанию)
    for (int i = 0; i < COLS; i++) {
        mvprintw(0, i, "#");
        mvprintw(LINES - 1, i, "#");
    }
    for (int i = 0; i < LINES; i++) {
        mvprintw(i, 0, "#");
        mvprintw(i, COLS - 1, "#");
    }

    // Отрисовка счета
    mvprintw(LINES - 2, 2, "Score: %d", score);
    mvprintw(LINES - 2, COLS - 30, "Press 'c' for color");
    mvprintw(LINES - 3, COLS - 30, "Press '+' or '-' for size");

    refresh(); // Обновить экран
}

void generate_food() {
    int valid_position = 0;
    while (!valid_position) {
        food.x = rand() % (COLS - 2) + 1; // -2 и +1 для отступа от границ
        food.y = rand() % (LINES - 2) + 1;

        valid_position = 1;
        // Проверка, чтобы еда не появилась на теле змейки
        for (int i = 0; i < snake.length; i++) {
            if (food.x == snake.body[i].x && food.y == snake.body[i].y) {
                valid_position = 0;
                break;
            }
        }
    }
}

void handle_input() {
    int ch = getch(); // Получить символ без ожидания

    switch (ch) {
        case KEY_RIGHT:
            if (snake.direction != 2) snake.direction = 0;
            break;
        case KEY_LEFT:
            if (snake.direction != 0) snake.direction = 2;
            break;
        case KEY_UP:
            if (snake.direction != 1) snake.direction = 3;
            break;
        case KEY_DOWN:
            if (snake.direction != 3) snake.direction = 1;
            break;
        case 'q':
            game_over = 1; // Выход из игры
            break;
        case '+': // Увеличить размер змейки (для тестирования/бонуса)
            if (snake.length < MAX_SNAKE_LENGTH) {
                // Добавляем новый элемент в конце, передвигая остальные
                for (int i = snake.length; i > 0; i--) {
                    snake.body[i] = snake.body[i - 1];
                }
                snake.length++;
            }
            break;
        case '-': // Уменьшить размер змейки (для тестирования/штрафа)
            if (snake.length > 1) {
                snake.length--;
            }
            break;
        case 'c': // Изменить цветовую палитру
            change_color_scheme();
            break;
    }
}

void update_game() {
    // Передвижение тела змейки (от хвоста к голове)
    for (int i = snake.length - 1; i > 0; i--) {
        snake.body[i] = snake.body[i - 1];
    }

    // Передвижение головы змейки
    switch (snake.direction) {
        case 0: snake.body[0].x++; break; // Вправо
        case 1: snake.body[0].y++; break; // Вниз
        case 2: snake.body[0].x--; break; // Влево
        case 3: snake.body[0].y--; break; // Вверх
    }

    // Проверка на столкновение со стенами
    if (snake.body[0].x <= 0 || snake.body[0].x >= COLS - 1 ||
        snake.body[0].y <= 0 || snake.body[0].y >= LINES - 1) {
        game_over = 1;
        return;
    }

    // Проверка на столкновение с собой
    for (int i = 1; i < snake.length; i++) {
        if (snake.body[0].x == snake.body[i].x && snake.body[0].y == snake.body[i].y) {
            game_over = 1;
            return;
        }
    }

    // Проверка на съедание еды
    if (snake.body[0].x == food.x && snake.body[0].y == food.y) {
        score += 10;
        grow_shrink_counter++; // Увеличиваем счетчик для роста/уменьшения
        generate_food();

        // Увеличиваем змейку
        if (snake.length < MAX_SNAKE_LENGTH) {
            snake.length++;
        }
        
        // Увеличиваем скорость игры (уменьшаем задержку)
        if (delay_ms > 50) { // Минимальная задержка
            delay_ms -= 5;
        }
    }
    
    // Механизм роста/уменьшения (например, каждый 5-й раз уменьшаем, если не съели еду)
    // Это пример реализации "уменьшения размеров змейки".
    // Можно привязать к прохождению времени без еды, или к специальному "отрицательному" объекту.
    // Здесь я привяжу это к счетчику: если 5 еды съедено, то уменьшим её.
    if (grow_shrink_counter >= 5 && snake.length > 3) { // Уменьшаем, если съели 5 еды и змейка достаточно длинная
        snake.length--;
        grow_shrink_counter = 0; // Сбрасываем счетчик
    }
}

void end_game() {
    clear();
    mvprintw(LINES / 2 - 2, COLS / 2 - 5, "GAME OVER!");
    mvprintw(LINES / 2, COLS / 2 - 7, "Final Score: %d", score);
    mvprintw(LINES / 2 + 2, COLS / 2 - 12, "Press any key to exit...");
    refresh();
    timeout(-1); // Блокирующий ввод
    getch();     // Ждем нажатия любой клавиши
    endwin();    // Завершение работы ncurses
}

void change_color_scheme() {
    color_scheme = (color_scheme + 1) % 3; // Переключение между 3 схемами
}