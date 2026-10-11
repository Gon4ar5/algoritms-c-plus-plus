// ============================================================
//  Задача коммивояжёра — метод имитации отжига
//  Логика разбита на понятные части, чтобы можно было объяснять
// ============================================================

#include <SFML/Graphics.hpp>
#include <random>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <iomanip>

// ---------- Шрифт для русского текста ----------
sf::String u8(const std::string& s) {
    return sf::String::fromUtf8(s.begin(), s.end());
}

// ДАННЫЕ

const int N = 12; // сколько городов

struct City { float x, y; };
std::vector<City> cities(N);

// Маршрут — это порядок, в котором коммивояжёр объезжает города.
// Например, tour = {3, 0, 5, 1, ...} значит: сначала город 3,
// потом 0, потом 5, и так далее, а в конце возвращаемся в 3.
std::vector<int> tour(N);
std::vector<int> bestTour(N);  // лучший маршрут за всё время

float currentLen = 0;          // длина текущего маршрута
float bestLen    = 0;          // длина лучшего маршрута

// «Температура» — главная величина метода отжига.
// Большая T = разрешаем плохие ходы.
// Маленькая T = принимаем только хорошие.
double T = 500.0;
const double T_MIN = 0.5;      // при такой T останавливаемся
const double COOL  = 0.97;     // во сколько раз уменьшаем T за шаг

std::mt19937 rng(std::random_device{}());

//  ПРАВИЛА ИГРЫ (чистая математика, без графики)
// Расстояние между двумя городами (теорема Пифагора)
float dist(const City& a, const City& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

// Длина замкнутого маршрута: сумма расстояний между соседями
float tourLength(const std::vector<int>& t) {
    float len = 0;
    for (int i = 0; i < N; ++i)
        len += dist(cities[t[i]], cities[t[(i + 1) % N]]);
    return len;
}

// ОДИН ШАГ ОТЖИГА
// Всё, что нужно знать о сделанном шаге — для отрисовки и объяснения
struct StepInfo {
    bool  done     = false;
    int   i = 0, j = 0;         // какие две позиции в маршруте выбрали
    int   a = 0, b = 0, c = 0, d = 0;  // четыре города вокруг разреза
    float oldLen   = 0;         // сколько стоили 2 разрезанных ребра
    float newLen   = 0;         // сколько будут стоить 2 новых ребра
    float delta    = 0;         // newLen - oldLen
    float prob     = 0;         // вероятность принять, если delta > 0
    bool  accepted = false;
};
StepInfo lastStep;

long long stepNumber = 0;

// Случайная начальная расстановка городов и случайный маршрут
void reset() {
    std::uniform_real_distribution<float> posX(80.f, 900.f);
    std::uniform_real_distribution<float> posY(120.f, 780.f);
    for (auto& c : cities) c = { posX(rng), posY(rng) };

    std::iota(tour.begin(), tour.end(), 0);
    std::shuffle(tour.begin(), tour.end(), rng);
    bestTour = tour;

    currentLen = tourLength(tour);
    bestLen    = currentLen;
    T = 500.0;
    stepNumber = 0;
    lastStep.done = false;
}

// Шаг 1: выбрать два ребра, которые будем «перерезать».
// Возвращаем позиции i и j в маршруте (i < j).
// Участок tour[i..j] будет развёрнут задом наперёд.
bool pickTwoEdges(int& i, int& j) {
    std::uniform_int_distribution<int> pick(0, N - 1);
    i = pick(rng);
    j = pick(rng);
    if (i > j) std::swap(i, j);

    // Нельзя брать одно и то же ребро или соседние
    if (i == j) return false;
    if (i == 0 && j == N - 1) return false;
    return true;
}

// Шаг 2: посчитать, на сколько изменится длина, если развернуть [i..j].
// Разрезаем два ребра и склеиваем по-новому. Меняются только они.
float deltaIfReverse(int i, int j) {
    int a = tour[i];
    int b = tour[(i - 1 + N) % N];
    int c = tour[j];
    int d = tour[(j + 1) % N];

    float oldLen = dist(cities[b], cities[a]) + dist(cities[c], cities[d]);
    float newLen = dist(cities[b], cities[c]) + dist(cities[a], cities[d]);
    return newLen - oldLen;
}

// Шаг 3: решить — принимаем ход или нет.
// Если ход уменьшает длину (delta < 0) — принимаем всегда.
// Если увеличивает — принимаем с вероятностью exp(-delta / T).
bool shouldAccept(float delta, float& probOut) {
    if (delta < 0) { probOut = 1.0f; return true; }
    double prob = std::exp(-delta / T);
    probOut = (float)prob;
    std::uniform_real_distribution<double> uni(0.0, 1.0);
    return uni(rng) < prob;
}

// Шаг 4: сделать один ход целиком
void makeOneStep() {
    if (T <= T_MIN) return;

    int i, j;
    if (!pickTwoEdges(i, j)) return;

    int a = tour[i];
    int b = tour[(i - 1 + N) % N];
    int c = tour[j];
    int d = tour[(j + 1) % N];

    float oldLen = dist(cities[b], cities[a]) + dist(cities[c], cities[d]);
    float newLen = dist(cities[b], cities[c]) + dist(cities[a], cities[d]);
    float delta  = newLen - oldLen;

    float prob = 0;
    bool accept = shouldAccept(delta, prob);

    if (accept) {
        std::reverse(tour.begin() + i, tour.begin() + j + 1);
        currentLen += delta;
        if (currentLen < bestLen) {
            bestLen = currentLen;
            bestTour = tour;
        }
    }

    // Запоминаем ход
    lastStep.done = true;
    lastStep.i = i; lastStep.j = j;
    lastStep.a = a; lastStep.b = b; lastStep.c = c; lastStep.d = d;
    lastStep.oldLen = oldLen;
    lastStep.newLen = newLen;
    lastStep.delta  = delta;
    lastStep.prob   = prob;
    lastStep.accepted = accept;

    T *= COOL;               // остываем
    ++stepNumber;
}

// ГРАФИКА
void drawThickLine(sf::RenderWindow& window,
                   float x1, float y1, float x2, float y2,
                   float th, sf::Color col) {
    sf::Vector2f a(x1, y1), b(x2, y2);
    sf::Vector2f d = b - a;
    float len = std::sqrt(d.x * d.x + d.y * d.y);
    if (len < 0.01f) return;
    sf::RectangleShape rect(sf::Vector2f(len, th));
    rect.setOrigin(0, th / 2);
    rect.setPosition(a);
    rect.setRotation(std::atan2(d.y, d.x) * 180.f / 3.14159265f);
    rect.setFillColor(col);
    window.draw(rect);
}

void drawDashedLine(sf::RenderWindow& window,
                    float x1, float y1, float x2, float y2,
                    float th, float dashLen, float gapLen,
                    sf::Color col) {
    float dx = x2 - x1, dy = y2 - y1;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.01f) return;
    float ux = dx / len, uy = dy / len;
    float t = 0;
    while (t < len) {
        float seg = std::min(dashLen, len - t);
        sf::RectangleShape rect(sf::Vector2f(seg, th));
        rect.setOrigin(0, th / 2);
        rect.setPosition(x1 + ux * t, y1 + uy * t);
        rect.setRotation(std::atan2(dy, dx) * 180.f / 3.14159265f);
        rect.setFillColor(col);
        window.draw(rect);
        t += dashLen + gapLen;
    }
}

void drawCity(sf::RenderWindow& window, sf::Font& font,
              const City& c, int num) {
    sf::CircleShape dot(20);
    dot.setOrigin(20, 20);
    dot.setFillColor(sf::Color(245, 245, 250));
    dot.setOutlineColor(sf::Color(60, 60, 90));
    dot.setOutlineThickness(3);
    dot.setPosition(c.x, c.y);
    window.draw(dot);

    sf::Text nt;
    nt.setFont(font);
    nt.setString(std::to_string(num + 1));
    nt.setCharacterSize(18);
    nt.setStyle(sf::Text::Bold);
    nt.setFillColor(sf::Color(30, 30, 50));
    sf::FloatRect b = nt.getLocalBounds();
    nt.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
    nt.setPosition(c.x, c.y);
    window.draw(nt);
}

// ГЛАВНАЯ ПРОГРАММА
int main() {
    const unsigned W = 1200, H = 900;
    sf::RenderWindow window(sf::VideoMode(W, H), "TSP - Simulated Annealing");
    window.setFramerateLimit(60);

    // ---------- Шрифты ----------
    sf::Font font;
    font.loadFromFile("/System/Library/Fonts/Supplemental/Arial.ttf");

    sf::Text info, big;
    info.setFont(font); info.setCharacterSize(16);
    info.setFillColor(sf::Color::White); info.setPosition(15, 15);
    big.setFont(font);  big.setCharacterSize(46); big.setStyle(sf::Text::Bold);

    auto fmt = [](float v) {
        std::ostringstream s;
        s << std::fixed << std::setprecision(1) << v;
        return s.str();
    };

    reset();

    // ---------- Тайминг ----------
    int delayMs = 800;
    bool autoMode = true;
    sf::Clock clock;

    // ================= ГЛАВНЫЙ ЦИКЛ =================
    while (window.isOpen()) {
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) window.close();
            if (e.type == sf::Event::KeyPressed) {
                if (e.key.code == sf::Keyboard::Escape) window.close();
                if (e.key.code == sf::Keyboard::Space)  makeOneStep();
                if (e.key.code == sf::Keyboard::Enter)  autoMode = !autoMode;
                if (e.key.code == sf::Keyboard::R)      reset();
                if (e.key.code == sf::Keyboard::Equal || e.key.code == sf::Keyboard::Add)
                    delayMs = std::max(delayMs / 2, 50);
                if (e.key.code == sf::Keyboard::Dash || e.key.code == sf::Keyboard::Subtract)
                    delayMs = std::min(delayMs * 2, 5000);
            }
        }

        if (autoMode && T > T_MIN &&
            clock.getElapsedTime().asMilliseconds() >= delayMs) {
            makeOneStep();
            clock.restart();
        }

        // ---------- Заголовок окна (латиница) ----------
        std::ostringstream title;
        title << "TSP | step " << stepNumber
              << " | T = " << (int)T
              << " | cur " << (int)currentLen
              << " | best " << (int)bestLen;
        window.setTitle(title.str());

        // ---------- Текст слева ----------
        std::ostringstream t;
        t << "Задача коммивояжёра\n"
          << "Городов: " << N << "\n\n"
          << "Шаг:           " << stepNumber << "\n"
          << "Температура T: " << fmt((float)T) << "\n"
          << "Сейчас длина:  " << fmt(currentLen) << "\n"
          << "Лучшая длина:  " << fmt(bestLen) << "\n\n";

        if (lastStep.done) {
            t << "ПОСЛЕДНИЙ ХОД:\n"
              << "  выбрали 2 ребра и разрезаем:\n"
              << "    ребро " << (lastStep.b + 1)
              << " — "        << (lastStep.a + 1) << "\n"
              << "    ребро " << (lastStep.c + 1)
              << " — "        << (lastStep.d + 1) << "\n"
              << "  и склеиваем по-новому:\n"
              << "    " << (lastStep.b + 1) << " — " << (lastStep.c + 1) << "\n"
              << "    " << (lastStep.a + 1) << " — " << (lastStep.d + 1) << "\n\n"
              << "  Было:   " << fmt(lastStep.oldLen) << "\n"
              << "  Станет: " << fmt(lastStep.newLen) << "\n"
              << "  Δ = " << fmt(lastStep.delta) << "\n"
              << "  P(принять) = " << fmt(lastStep.prob) << "\n\n";
            if (lastStep.accepted) {
                t << "  РЕЗУЛЬТАТ: ПРИНЯТ\n";
            } else {
                t << "  РЕЗУЛЬТАТ: ОТКЛОНЁН\n";
            }
            t << "\n";
        }

        t << "ПОЧЕМУ ТАК:\n"
          << "  Если ход укорачивает путь —\n"
          << "  берём всегда. Если удлиняет —\n"
          << "  берём с вероятностью exp(-Δ/T).\n"
          << "  Пока T большая — берём даже\n"
          << "  плохие ходы, чтобы не застрять.\n"
          << "  Потом T падает, и путь «застывает».\n\n"
          << "ОБОЗНАЧЕНИЯ:\n"
          << "  красный пунктир = разрезаем\n"
          << "  жёлтый жирный   = склеиваем\n\n"
          << "[Space] один ход\n"
          << "[Enter] авто/ручной\n"
          << "[+/-]   скорость\n"
          << "[R]     новые города";
        info.setString(u8(t.str()));

        // ================= РИСОВАНИЕ =================
        window.clear(sf::Color(25, 25, 40));

        // 1) Лучший маршрут — тонкий зелёный
        for (int i = 0; i < N; ++i) {
            int u = bestTour[i], v = bestTour[(i + 1) % N];
            drawThickLine(window,
                cities[u].x, cities[u].y, cities[v].x, cities[v].y,
                2.f, sf::Color(60, 200, 110, 130));
        }

        // 2) Текущий маршрут — толстый серо-синий
        for (int i = 0; i < N; ++i) {
            int u = tour[i], v = tour[(i + 1) % N];
            drawThickLine(window,
                cities[u].x, cities[u].y, cities[v].x, cities[v].y,
                4.f, sf::Color(170, 170, 210));
        }

        // 3) Подсветка последнего хода
        if (lastStep.done) {
            // разрезаемые рёбра — красный пунктир
            drawDashedLine(window,
                cities[lastStep.b].x, cities[lastStep.b].y,
                cities[lastStep.a].x, cities[lastStep.a].y,
                7.f, 14.f, 10.f, sf::Color(255, 70, 70));
            drawDashedLine(window,
                cities[lastStep.c].x, cities[lastStep.c].y,
                cities[lastStep.d].x, cities[lastStep.d].y,
                7.f, 14.f, 10.f, sf::Color(255, 70, 70));

            // новые рёбра — жёлтый жирный
            sf::Color newCol = lastStep.accepted
                ? sf::Color(255, 230, 60)
                : sf::Color(180, 180, 90, 130);
            drawThickLine(window,
                cities[lastStep.b].x, cities[lastStep.b].y,
                cities[lastStep.c].x, cities[lastStep.c].y,
                6.f, newCol);
            drawThickLine(window,
                cities[lastStep.a].x, cities[lastStep.a].y,
                cities[lastStep.d].x, cities[lastStep.d].y,
                6.f, newCol);

            // кружки на концах
            sf::CircleShape ring(30);
            ring.setOrigin(30, 30);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(4);
            ring.setOutlineColor(sf::Color(255, 255, 100));
            ring.setPosition(cities[lastStep.a].x, cities[lastStep.a].y);
            window.draw(ring);
            ring.setPosition(cities[lastStep.c].x, cities[lastStep.c].y);
            window.draw(ring);
        }

        // 4) Города
        for (int i = 0; i < N; ++i)
            drawCity(window, font, cities[i], i);

        // 5) Большой статус внизу
        if (!lastStep.done) {
            big.setString(u8("ГОТОВО"));
            big.setFillColor(sf::Color(180, 180, 180));
        } else if (lastStep.accepted) {
            big.setString(u8("ХОД ПРИНЯТ"));
            big.setFillColor(sf::Color(80, 230, 120));
        } else {
            big.setString(u8("ХОД ОТКЛОНЁН"));
            big.setFillColor(sf::Color(230, 90, 90));
        }
        sf::FloatRect bb = big.getLocalBounds();
        big.setOrigin(bb.left + bb.width / 2.f, bb.top + bb.height / 2.f);
        big.setPosition(600.f, 855.f);
        window.draw(big);

        window.draw(info);
        window.display();
    }

    return 0;
}