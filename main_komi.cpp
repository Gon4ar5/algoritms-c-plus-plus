#include <SFML/Graphics.hpp>
#include <random>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <iomanip>

struct City { float x, y; };

float dist(const City& a, const City& b) {
    float dx = a.x - b.x, dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

float tourLength(const std::vector<int>& t, const std::vector<City>& c) {
    int n = t.size(); float L = 0;
    for (int i = 0; i < n; ++i) L += dist(c[t[i]], c[t[(i + 1) % n]]);
    return L;
}

// ---- Помощник: std::string (UTF-8) -> sf::String ----
sf::String u8(const std::string& s) {
    return sf::String::fromUtf8(s.begin(), s.end());
}

int main() {
    const unsigned W = 1200, H = 900;
    // В setTitle — только латиница (иначе SFML 2.x на macOS падает)
    sf::RenderWindow window(sf::VideoMode(W, H), "TSP - Simulated Annealing");
    window.setFramerateLimit(60);

    // ---------- Города ----------
    const int N = 12;
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> posX(80.f, 900.f);
    std::uniform_real_distribution<float> posY(120.f, 780.f);

    std::vector<City> cities(N);
    for (auto& c : cities) c = { posX(rng), posY(rng) };

    // ---------- Маршрут ----------
    std::vector<int> tour(N);
    std::iota(tour.begin(), tour.end(), 0);
    std::shuffle(tour.begin(), tour.end(), rng);
    std::vector<int> bestTour = tour;
    float currentLen = tourLength(tour, cities);
    float bestLen    = currentLen;

    // ---------- Отжиг ----------
    double T = 500.0;
    const double Tmin  = 0.5;
    const double alpha = 0.97;

    std::uniform_real_distribution<double> uni(0.0, 1.0);
    std::uniform_int_distribution<int>    idxDist(0, N - 1);

    long long step = 0;

    // ---------- Тайминг ----------
    int stepIntervalMs = 800;
    bool autoMode = true;
    sf::Clock clock;

    // ---------- Инфо о последнем ходе ----------
    bool hasMove = false;
    int  lastA = 0, lastB = 0, lastC = 0, lastD = 0;
    bool lastAccepted = false;
    float lastOldE = 0, lastNewE = 0, lastDelta = 0, lastProb = 0;

    // ---------- Шрифт ----------
    sf::Font font;
    font.loadFromFile("/System/Library/Fonts/Supplemental/Arial.ttf");

    sf::Text info;
    info.setFont(font);
    info.setCharacterSize(17);
    info.setFillColor(sf::Color::White);
    info.setPosition(15, 15);

    auto fmt = [](float v) {
        std::ostringstream s;
        s << std::fixed << std::setprecision(1) << v;
        return s.str();
    };

    // ---------- Рисовалки ----------
    auto drawThickLine = [&](float x1, float y1, float x2, float y2,
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
    };

    auto drawDashedLine = [&](float x1, float y1, float x2, float y2,
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
    };

    auto drawCity = [&](const City& c, int num, sf::Color fill, sf::Color outline) {
        sf::CircleShape dot(20);
        dot.setOrigin(20, 20);
        dot.setFillColor(fill);
        dot.setOutlineColor(outline);
        dot.setOutlineThickness(3);
        dot.setPosition(c.x, c.y);
        window.draw(dot);

        sf::Text nt;
        nt.setFont(font);
        nt.setString(std::to_string(num + 1)); // цифры — ASCII, u8 не нужен
        nt.setCharacterSize(18);
        nt.setStyle(sf::Text::Bold);
        nt.setFillColor(sf::Color(30, 30, 50));
        sf::FloatRect b = nt.getLocalBounds();
        nt.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
        nt.setPosition(c.x, c.y);
        window.draw(nt);
    };

    // ---------- Один ход отжига ----------
    auto doOneStep = [&]() -> bool {
        if (T <= Tmin) return false;

        int i = idxDist(rng);
        int j = idxDist(rng);
        if (i > j) std::swap(i, j);
        if (i == j || (i == 0 && j == N - 1)) return false;

        int a = tour[i];
        int b = tour[(i - 1 + N) % N];
        int c = tour[j];
        int d = tour[(j + 1) % N];

        float oldE = dist(cities[b], cities[a]) + dist(cities[c], cities[d]);
        float newE = dist(cities[b], cities[c]) + dist(cities[a], cities[d]);
        float delta = newE - oldE;

        double prob = (delta < 0.f) ? 1.0 : std::exp(-delta / T);
        bool accept = (delta < 0.f) || (uni(rng) < prob);

        lastA = a; lastB = b; lastC = c; lastD = d;
        lastOldE = oldE; lastNewE = newE;
        lastDelta = delta;
        lastProb  = (float)prob;
        lastAccepted = accept;
        hasMove = true;

        if (accept) {
            std::reverse(tour.begin() + i, tour.begin() + j + 1);
            currentLen += delta;
            if (currentLen < bestLen) {
                bestLen = currentLen;
                bestTour = tour;
            }
        }
        T *= alpha;
        ++step;
        return true;
    };

    // ---------- Главный цикл ----------
    while (window.isOpen()) {
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) window.close();
            if (e.type == sf::Event::KeyPressed) {
                if (e.key.code == sf::Keyboard::Escape) window.close();
                if (e.key.code == sf::Keyboard::Space)  doOneStep();
                if (e.key.code == sf::Keyboard::Enter)  autoMode = !autoMode;
                if (e.key.code == sf::Keyboard::R) {
                    for (auto& c : cities) c = { posX(rng), posY(rng) };
                    std::shuffle(tour.begin(), tour.end(), rng);
                    bestTour = tour;
                    currentLen = tourLength(tour, cities);
                    bestLen = currentLen;
                    T = 500.0;
                    step = 0;
                    hasMove = false;
                }
                if (e.key.code == sf::Keyboard::Equal || e.key.code == sf::Keyboard::Add)
                    stepIntervalMs = std::max(stepIntervalMs / 2, 50);
                if (e.key.code == sf::Keyboard::Dash || e.key.code == sf::Keyboard::Subtract)
                    stepIntervalMs = std::min(stepIntervalMs * 2, 5000);
            }
        }

        if (autoMode && clock.getElapsedTime().asMilliseconds() >= stepIntervalMs) {
            doOneStep();
            clock.restart();
        }

        // ---------- Заголовок окна (ТОЛЬКО ЛАТИНИЦА!) ----------
        std::ostringstream title;
        title << "TSP | step " << step
              << " | T = " << (int)T
              << " | cur " << (int)currentLen
              << " | best " << (int)bestLen;
        window.setTitle(title.str());

        // ---------- Инфо слева (u8!) ----------
        std::ostringstream t;
        t << "Шаг:            " << step << "\n"
          << "Температура T:  " << fmt((float)T) << "\n"
          << "Длина сейчас:   " << fmt(currentLen) << "\n"
          << "Лучшая длина:   " << fmt(bestLen) << "\n\n";

        if (hasMove) {
            t << "ПРОБУЕМ ХОД:\n"
              << "  развернуть участок\n"
              << "  от города " << (lastA + 1)
              << " до города " << (lastC + 1) << "\n\n"
              << "  Длина двух разрезаемых\n"
              << "  рёбер:  " << fmt(lastOldE) << "\n"
              << "  Длина двух новых\n"
              << "  рёбер:  " << fmt(lastNewE) << "\n"
              << "  Разница Δ = " << fmt(lastDelta) << "\n"
              << "  Вероятность\n"
              << "  принять = " << fmt(lastProb) << "\n\n"
              << (lastAccepted ? "РЕЗУЛЬТАТ: ПРИНЯТ\n"
                               : "РЕЗУЛЬТАТ: ОТКЛОНЁН\n");
        }
        t << "\nОбозначения:\n"
          << "  красный пунктир = разрезаем\n"
          << "  жёлтый жирный   = появятся\n\n"
          << "[Space] один ход вручную\n"
          << "[Enter] авто / ручной режим\n"
          << "[+/-]  скорость\n"
          << "[R]    новые города";
        info.setString(u8(t.str()));   // <-- ВОТ ЗДЕСЬ КОНВЕРТАЦИЯ

        // ---------- Рисование ----------
        window.clear(sf::Color(25, 25, 40));

        // 1) Лучший маршрут
        for (int i = 0; i < N; ++i) {
            int u = bestTour[i], v = bestTour[(i + 1) % N];
            drawThickLine(cities[u].x, cities[u].y,
                          cities[v].x, cities[v].y,
                          2.f, sf::Color(60, 200, 110, 130));
        }

        // 2) Текущий маршрут
        for (int i = 0; i < N; ++i) {
            int u = tour[i], v = tour[(i + 1) % N];
            drawThickLine(cities[u].x, cities[u].y,
                          cities[v].x, cities[v].y,
                          4.f, sf::Color(170, 170, 210));
        }

        // 3) Подсветка последнего хода
        if (hasMove) {
            drawDashedLine(cities[lastB].x, cities[lastB].y,
                           cities[lastA].x, cities[lastA].y,
                           7.f, 14.f, 10.f, sf::Color(255, 70, 70));
            drawDashedLine(cities[lastC].x, cities[lastC].y,
                           cities[lastD].x, cities[lastD].y,
                           7.f, 14.f, 10.f, sf::Color(255, 70, 70));

            sf::Color newCol = lastAccepted
                ? sf::Color(255, 230, 60)
                : sf::Color(180, 180, 90, 130);
            drawThickLine(cities[lastB].x, cities[lastB].y,
                          cities[lastC].x, cities[lastC].y,
                          6.f, newCol);
            drawThickLine(cities[lastA].x, cities[lastA].y,
                          cities[lastD].x, cities[lastD].y,
                          6.f, newCol);

            sf::CircleShape ring(30);
            ring.setOrigin(30, 30);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(4);
            ring.setOutlineColor(sf::Color(255, 255, 100));
            ring.setPosition(cities[lastA].x, cities[lastA].y);
            window.draw(ring);
            ring.setPosition(cities[lastC].x, cities[lastC].y);
            window.draw(ring);
        }

        // 4) Города с номерами
        for (int i = 0; i < N; ++i)
            drawCity(cities[i], i, sf::Color(245, 245, 250),
                     sf::Color(60, 60, 90));

        // 5) Большой статус внизу (u8!)
        sf::Text big;
        big.setFont(font);
        big.setCharacterSize(52);
        big.setStyle(sf::Text::Bold);
        if (!hasMove) {
            big.setString(u8("ГОТОВО"));
            big.setFillColor(sf::Color(180, 180, 180));
        } else if (lastAccepted) {
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