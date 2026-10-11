// ============================================================
//  Задача о N ферзях — метод min-conflicts
//  Логика разбита на понятные части, чтобы можно было объяснять
// ============================================================

#include <SFML/Graphics.hpp>
#include <random>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>

// ---------- Шрифт для русского текста ----------
sf::String u8(const std::string& s) {
    return sf::String::fromUtf8(s.begin(), s.end());
}

//  ДАННЫЕ
const int N = 8; // размер доски

// Расстановка ферзей: queen[col] = row
// То есть в столбце col ферзь стоит в строке row.
std::vector<int> queen(N);

std::mt19937 rng(std::random_device{}());

// Бьют ли два ферзя друг друга?
bool attacks(int colA, int colB) {
    if (colA == colB) return false;

    int rowA = queen[colA];
    int rowB = queen[colB];

    // одна и та же строка
    if (rowA == rowB) return true;

    // одна и та же диагональ: |Δстрока| == |Δстолбец|
    if (std::abs(rowA - rowB) == std::abs(colA - colB)) return true;

    return false;
}

// Сколько ферзей бьёт ферзя в столбце col
int conflictsOfQueen(int col) {
    int cnt = 0;
    for (int other = 0; other < N; ++other)
        if (attacks(col, other)) cnt++;
    return cnt;
}

// Сколько ВСЕГО бьющих пар на доске
int totalConflicts() {
    int s = 0;
    for (int i = 0; i < N; ++i)
        s += conflictsOfQueen(i);
    return s / 2;   // каждую пару посчитали дважды
}

// ОДИН ШАГ АЛГОРИТМА
// Всё, что нужно знать о сделанном шаге — для отрисовки и объяснения
struct StepInfo {
    bool done = false;         // был ли вообще ход
    int  col  = -1;            // столбец, где двигали ферзя
    int  from = -1;            // из какой строки
    int  to   = -1;            // в какую строку
    std::vector<int> score;    // score[r] = сколько конфликтов у ферзя, если поставить его в строку r
};

StepInfo lastStep;

// Случайная начальная расстановка
void reset() {
    std::uniform_int_distribution<int> randRow(0, N - 1);
    for (int i = 0; i < N; ++i)
        queen[i] = randRow(rng);
    lastStep.done = false;
}

// Шаг 1: выбрать ферзя, который кого-то бьёт
int pickConflictedQueen() {
    std::vector<int> bad;
    for (int i = 0; i < N; ++i)
        if (conflictsOfQueen(i) > 0) bad.push_back(i);

    if (bad.empty()) return -1;   // все ферзи уже не бьются

    std::uniform_int_distribution<int> pick(0, (int)bad.size() - 1);
    return bad[pick(rng)];
}

// Шаг 2: посчитать, сколько конфликтов будет у ферзя в каждой строке
// столбца col
std::vector<int> scoreRows(int col) {
    std::vector<int> score(N, 0);
    int saved = queen[col];

    for (int r = 0; r < N; ++r) {
        queen[col] = r;                 // временно ставим в строку r
        score[r] = conflictsOfQueen(col);
    }

    queen[col] = saved;                 // возвращаем как было
    return score;
}

// Шаг 3: из всех строк выбрать те, где конфликтов меньше всего
std::vector<int> bestRows(const std::vector<int>& score, int skipRow) {
    int best = 1000;
    for (int r = 0; r < N; ++r) {
        if (r == skipRow) continue;     // не рассматриваем текущую строку
        if (score[r] < best) best = score[r];
    }

    std::vector<int> result;
    for (int r = 0; r < N; ++r)
        if (r != skipRow && score[r] == best) result.push_back(r);

    return result;
}

// Шаг 4: сделать один ход целиком — собрать всё вместе
void makeOneStep() {
    if (totalConflicts() == 0) return;      // уже решено

    // 1. выбираем "плохого" ферзя
    int col = pickConflictedQueen();
    if (col < 0) return;
    int from = queen[col];

    // 2. считаем, сколько конфликтов было бы в каждой строке
    std::vector<int> score = scoreRows(col);

    // 3. находим лучшие строки
    std::vector<int> best = bestRows(score, from);
    if (best.empty()) best.push_back(from);  // страховка

    // 4. случайно выбираем одну из лучших
    std::uniform_int_distribution<int> pick(0, (int)best.size() - 1);
    int to = best[pick(rng)];

    // 5. двигаем ферзя
    queen[col] = to;

    // 6. запоминаем ход
    lastStep.done  = true;
    lastStep.col   = col;
    lastStep.from  = from;
    lastStep.to    = to;
    lastStep.score = score;
}

// ============================================================
//  ЧАСТЬ 4. ГРАФИКА
// ============================================================

// Нарисовать ферзя
void drawQueen(sf::RenderWindow& window, float cx, float cy, float s, sf::Color col) {
    // тело
    sf::CircleShape body(s * 0.28f);
    body.setOrigin(s * 0.28f, s * 0.28f);
    body.setPosition(cx, cy + s * 0.05f);
    body.setFillColor(col);
    window.draw(body);

    // корона — 3 шарика
    for (int i = -1; i <= 1; ++i) {
        sf::CircleShape dot(s * 0.09f);
        dot.setOrigin(s * 0.09f, s * 0.09f);
        dot.setPosition(cx + i * s * 0.18f, cy - s * 0.26f);
        dot.setFillColor(col);
        window.draw(dot);
    }

    // основание
    sf::RectangleShape base(sf::Vector2f(s * 0.55f, s * 0.10f));
    base.setOrigin(s * 0.275f, s * 0.05f);
    base.setPosition(cx, cy + s * 0.32f);
    base.setFillColor(col);
    window.draw(base);
}

int main() {
    const unsigned W = 1200, H = 900;
    sf::RenderWindow window(sf::VideoMode(W, H), "N-Queens (min-conflicts)");
    window.setFramerateLimit(60);

    const float cell   = 80.f;
    const float boardX = 500.f;
    const float boardY = 130.f;

    // ---------- Шрифты ----------
    sf::Font font;
    font.loadFromFile("/System/Library/Fonts/Supplemental/Arial.ttf");

    sf::Text info, big, num;
    info.setFont(font);  info.setCharacterSize(16);
    info.setFillColor(sf::Color::White); info.setPosition(20, 20);
    big.setFont(font);   big.setCharacterSize(46); big.setStyle(sf::Text::Bold);
    num.setFont(font);   num.setCharacterSize(22); num.setStyle(sf::Text::Bold);

    reset();

    // ---------- Тайминг ----------
    int delayMs = 1500;
    bool autoMode = true;
    sf::Clock clock;
    long long iter = 0;

    // ================= ГЛАВНЫЙ ЦИКЛ =================
    while (window.isOpen()) {
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) window.close();
            if (e.type == sf::Event::KeyPressed) {
                if (e.key.code == sf::Keyboard::Escape) window.close();
                if (e.key.code == sf::Keyboard::Space)  { makeOneStep(); ++iter; }
                if (e.key.code == sf::Keyboard::Enter)  autoMode = !autoMode;
                if (e.key.code == sf::Keyboard::R)      { reset(); iter = 0; }
                if (e.key.code == sf::Keyboard::Equal || e.key.code == sf::Keyboard::Add)
                    delayMs = std::max(delayMs / 2, 200);
                if (e.key.code == sf::Keyboard::Dash || e.key.code == sf::Keyboard::Subtract)
                    delayMs = std::min(delayMs * 2, 5000);
            }
        }

        int conf = totalConflicts();
        if (autoMode && conf > 0 &&
            clock.getElapsedTime().asMilliseconds() >= delayMs) {
            makeOneStep();
            ++iter;
            clock.restart();
        }

        // ---------- Заголовок окна ----------
        std::ostringstream title;
        title << "N-Queens | step " << iter << " | conflicts " << conf;
        window.setTitle(title.str());

        // ---------- Текст слева ----------
        std::ostringstream t;
        t << "Задача о ферзях\n"
          << "Доска: " << N << "x" << N << "\n\n"
          << "Шаг:         " << iter << "\n"
          << "Конфликтов:  " << conf << "\n"
          << "Решено:      " << (conf == 0 ? "ДА" : "нет") << "\n\n";

        if (lastStep.done) {
            t << "ЧТО СДЕЛАЛ АЛГОРИТМ:\n"
              << "  1) выбрал ферзя, который\n"
              << "     кого-то бьёт\n"
              << "  2) примерил его во все строки\n"
              << "     своего столбца\n"
              << "  3) посчитал конфликты в каждой\n"
              << "     клетке (числа на доске)\n"
              << "  4) поставил туда, где меньше\n\n"
              << "  столбец: " << (lastStep.col + 1) << "\n"
              << "  было:    строка " << (lastStep.from + 1) << "\n"
              << "  стало:   строка " << (lastStep.to + 1) << "\n\n";
        }

        t << "ОБОЗНАЧЕНИЯ:\n"
          << "  белый    — никого не бьёт\n"
          << "  красный  — кого-то бьёт\n"
          << "  жёлтый   — сейчас двигали\n"
          << "  серый    — где стоял\n"
          << "  цифра    — сколько конфликтов\n"
          << "             было бы здесь\n"
          << "  зел.рамка — выбрали эту клетку\n\n"
          << "[Space]  один ход\n"
          << "[Enter]  авто/ручной\n"
          << "[+/-]    скорость\n"
          << "[R]      заново";
        info.setString(u8(t.str()));

        // ================= РИСОВАНИЕ =================
        window.clear(sf::Color(30, 30, 50));

        // Доска
        for (int c = 0; c < N; ++c)
            for (int r = 0; r < N; ++r) {
                sf::RectangleShape sq(sf::Vector2f(cell, cell));
                sq.setPosition(boardX + c * cell, boardY + r * cell);
                bool light = ((c + r) % 2 == 0);
                sq.setFillColor(light ? sf::Color(240, 218, 180)
                                      : sf::Color(120, 80, 50));
                window.draw(sq);
            }

        // Подсветка столбца
        if (lastStep.done) {
            sf::RectangleShape colHi(sf::Vector2f(cell, cell * N));
            colHi.setPosition(boardX + lastStep.col * cell, boardY);
            colHi.setFillColor(sf::Color(255, 230, 60, 30));
            window.draw(colHi);
        }

        // Серый кружок — где ферзь стоял
        if (lastStep.done && lastStep.from != lastStep.to) {
            sf::CircleShape ghost(cell * 0.28f);
            ghost.setOrigin(cell * 0.28f, cell * 0.28f);
            ghost.setPosition(boardX + lastStep.col * cell + cell / 2.f,
                              boardY + lastStep.from * cell + cell / 2.f);
            ghost.setFillColor(sf::Color(150, 150, 150, 130));
            ghost.setOutlineColor(sf::Color(200, 200, 200, 200));
            ghost.setOutlineThickness(2);
            window.draw(ghost);
        }

        // Зелёная рамка — куда поставили
        if (lastStep.done && lastStep.from != lastStep.to) {
            sf::RectangleShape hi(sf::Vector2f(cell, cell));
            hi.setPosition(boardX + lastStep.col * cell,
                           boardY + lastStep.to  * cell);
            hi.setFillColor(sf::Color::Transparent);
            hi.setOutlineColor(sf::Color(80, 230, 120));
            hi.setOutlineThickness(4);
            window.draw(hi);
        }

        // Числа в клетках столбца
        if (lastStep.done) {
            for (int r = 0; r < N; ++r) {
                float x = boardX + lastStep.col * cell + cell / 2.f;
                float y = boardY + r * cell + cell / 2.f;

                sf::CircleShape badge(cell * 0.20f);
                badge.setOrigin(cell * 0.20f, cell * 0.20f);
                badge.setPosition(x, y);

                int sc = lastStep.score[r];
                sf::Color bc;
                if (r == lastStep.to)         bc = sf::Color(80, 220, 120);
                else if (r == lastStep.from)  bc = sf::Color(160, 160, 160);
                else if (sc == 0)             bc = sf::Color(140, 230, 150);
                else if (sc == 1)             bc = sf::Color(255, 210, 70);
                else                          bc = sf::Color(235, 90, 90);
                badge.setFillColor(bc);
                badge.setOutlineColor(sf::Color(20, 20, 20));
                badge.setOutlineThickness(2);
                window.draw(badge);

                num.setString(std::to_string(sc));
                num.setFillColor(sf::Color(20, 20, 20));
                sf::FloatRect b = num.getLocalBounds();
                num.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
                num.setPosition(x, y);
                window.draw(num);
            }
        }

        // Ферзи
        for (int i = 0; i < N; ++i) {
            float cx = boardX + i * cell + cell / 2.f;
            float cy = boardY + queen[i] * cell + cell / 2.f;

            sf::Color col(245, 245, 250);
            if (conf == 0)                       col = sf::Color(80, 230, 120);
            else if (lastStep.done && i == lastStep.col)
                                                 col = sf::Color(255, 230, 60);
            else if (conflictsOfQueen(i) > 0)    col = sf::Color(235, 70, 70);

            drawQueen(window, cx, cy, cell, col);
        }

        // Рамка доски
        sf::RectangleShape border(sf::Vector2f(cell * N, cell * N));
        border.setPosition(boardX, boardY);
        border.setFillColor(sf::Color::Transparent);
        border.setOutlineColor(sf::Color(220, 200, 160));
        border.setOutlineThickness(3);
        window.draw(border);

        // Большая надпись
        if (conf == 0) {
            big.setString(u8("РЕШЕНО!"));
            big.setFillColor(sf::Color(80, 230, 120));
        } else {
            big.setString(u8("Идёт поиск..."));
            big.setFillColor(sf::Color(255, 230, 60));
        }
        sf::FloatRect bb = big.getLocalBounds();
        big.setOrigin(bb.left + bb.width / 2.f, bb.top + bb.height / 2.f);
        big.setPosition(boardX + cell * N / 2.f, boardY + cell * N + 50.f);
        window.draw(big);

        window.draw(info);
        window.display();
    }

    return 0;
}