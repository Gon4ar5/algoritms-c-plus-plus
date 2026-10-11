// ============================================================
//  Вычисление числа π методом Монте-Карло
//  Логика разбита на понятные части, чтобы можно было объяснять
// ============================================================

#include <SFML/Graphics.hpp>
#include <random>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>

// ---------- Шрифт для русского текста (SFML 2.x на macOS) ----------
sf::String u8(const std::string& s) {
    return sf::String::fromUtf8(s.begin(), s.end());
}

// ДАННЫЕ
// Мысленный квадрат [0,1] x [0,1] и вписанная в него
// четверть круга радиуса 1 с центром в (0,0).

struct Point { float x, y; };   // точка на экране

std::vector<Point> insidePts;   // точки, попавшие в круг
std::vector<Point> outsidePts;  // точки, попавшие мимо

long long total  = 0;           // сколько точек брошено
long long inside = 0;           // сколько попало в круг

std::mt19937 rng(std::random_device{}());

// ПРАВИЛА ИГРЫ (чистая математика, без графики)
// Попала ли точка (x, y) в четверть круга?
// Уравнение окружности радиуса 1: x² + y² = 1
// Внутри: x² + y² ≤ 1
bool isInsideCircle(float x, float y) {
    return (x * x + y * y) <= 1.0f;
}

// Оценка числа π по результатам бросков.
// Площадь четверти круга = π/4, площадь квадрата = 1.
// Значит доля точек внутри ≈ π/4, откуда π ≈ 4 * inside / total.
double computePi(long long total, long long inside) {
    if (total == 0) return 0.0;
    return 4.0 * (double)inside / (double)total;
}

// ОДИН БРОСОК
// Бросаем одну точку: генерируем случайные (x, y) в [0,1]×[0,1],
// проверяем, попала ли она в круг, и обновляем счётчики.
Point throwOnePoint() {
    std::uniform_real_distribution<float> d(0.0f, 1.0f);
    float x = d(rng);
    float y = d(rng);

    Point p{ x, y };
    if (isInsideCircle(x, y)) {
        ++inside;
        insidePts.push_back(p);
    } else {
        outsidePts.push_back(p);
    }
    ++total;
    return p;
}

void reset() {
    insidePts.clear();
    outsidePts.clear();
    total  = 0;
    inside = 0;
}

// ГРАФИКА
const unsigned W = 900, H = 900;
const float MARGIN = 60.f;
const float SIZE   = std::min(W, H) - 2 * MARGIN;
const float OX     = (W - SIZE) / 2.f;
const float OY     = (H - SIZE) / 2.f;

// Из математических координат [0,1]×[0,1] — в экранные
// (ось Y на экране смотрит вниз, поэтому переворачиваем)
sf::Vector2f toScreen(const Point& p) {
    return sf::Vector2f(
        OX + p.x * SIZE,
        OY + (1.f - p.y) * SIZE
    );
}

// ГЛАВНАЯ ПРОГРАММА
int main() {
    sf::RenderWindow window(sf::VideoMode(W, H), "Monte-Carlo Pi");
    window.setFramerateLimit(60);

    // ---------- Рамка квадрата ----------
    sf::RectangleShape square(sf::Vector2f(SIZE, SIZE));
    square.setPosition(OX, OY);
    square.setFillColor(sf::Color::Transparent);
    square.setOutlineColor(sf::Color::White);
    square.setOutlineThickness(2.f);

    // ---------- Дуга четверти круга ----------
    const int ARC_SEG = 400;
    sf::VertexArray arc(sf::LineStrip, ARC_SEG + 1);
    for (int i = 0; i <= ARC_SEG; ++i) {
        float t   = (float)i / ARC_SEG;
        float ang = t * 3.14159265358979f / 2.f;
        Point p{ std::cos(ang), std::sin(ang) };
        arc[i].position = toScreen(p);
        arc[i].color    = sf::Color(120, 200, 255);
    }

    // ---------- Шрифты ----------
    sf::Font font;
    font.loadFromFile("/System/Library/Fonts/Supplemental/Arial.ttf");

    sf::Text info, big;
    info.setFont(font);  info.setCharacterSize(16);
    info.setFillColor(sf::Color::White); info.setPosition(15, 15);
    big.setFont(font);   big.setCharacterSize(38);
    big.setStyle(sf::Text::Bold);

    reset();

    // ---------- Скорость ----------
    int  pointsPerFrame = 200;   // сколько точек бросаем за кадр
    bool autoMode       = true;

    // ================= ГЛАВНЫЙ ЦИКЛ =================
    while (window.isOpen()) {
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) window.close();
            if (e.type == sf::Event::KeyPressed) {
                if (e.key.code == sf::Keyboard::Escape) window.close();
                if (e.key.code == sf::Keyboard::Space)  {
                    for (int k = 0; k < pointsPerFrame; ++k) throwOnePoint();
                }
                if (e.key.code == sf::Keyboard::Enter)  autoMode = !autoMode;
                if (e.key.code == sf::Keyboard::R)      reset();
                if (e.key.code == sf::Keyboard::Equal || e.key.code == sf::Keyboard::Add)
                    pointsPerFrame = std::min(pointsPerFrame * 2, 20000);
                if (e.key.code == sf::Keyboard::Dash || e.key.code == sf::Keyboard::Subtract)
                    pointsPerFrame = std::max(pointsPerFrame / 2, 1);
            }
        }

        // ---------- Бросаем точки ----------
        if (autoMode) {
            for (int k = 0; k < pointsPerFrame; ++k) throwOnePoint();
        }

        double pi = computePi(total, inside);

        // ---------- Заголовок окна (латиница) ----------
        std::ostringstream title;
        title << "Monte-Carlo Pi | N=" << total
              << " | inside=" << inside
              << " | pi ~ " << pi;
        window.setTitle(title.str());

        // ---------- Текст слева ----------
        std::ostringstream t;
        t << "Вычисление числа π\n\n"
          << "Брошено точек:    " << total  << "\n"
          << "Попало в круг:    " << inside << "\n"
          << "π приблизительно: " << pi << "\n"
          << "Точное значение:  3.14159265...\n\n"
          << "ЧТО МЫ ДЕЛАЕМ:\n"
          << "  Бросаем случайные точки в квадрат.\n"
          << "  Квадрат у нас площадью 1,\n"
          << "  а четверть круга — площадью π/4.\n\n"
          << "  Значит доля точек, попавших в круг,\n"
          << "  примерно равна π/4.\n"
          << "  Отсюда:  π ≈ 4 · (в круге) / (всего)\n\n"
          << "  Чем больше точек бросим,\n"
          << "  тем точнее получится π.\n\n"
          << "ОБОЗНАЧЕНИЯ:\n"
          << "  зелёные точки — попали в круг\n"
          << "  красные точки — попали мимо\n\n"
          << "[Space]  бросить порцию точек\n"
          << "[Enter]  авто / пауза\n"
          << "[+/-]    сколько бросать за кадр\n"
          << "[R]      начать заново";
        info.setString(u8(t.str()));

        // ================= РИСОВАНИЕ =================
        window.clear(sf::Color(20, 20, 30));

        // Точки — одним вызовом через VertexArray (быстро)
        sf::VertexArray vout(sf::Points, outsidePts.size());
        for (size_t i = 0; i < outsidePts.size(); ++i) {
            vout[i].position = toScreen(outsidePts[i]);
            vout[i].color    = sf::Color(230, 80, 80);
        }
        sf::VertexArray vin(sf::Points, insidePts.size());
        for (size_t i = 0; i < insidePts.size(); ++i) {
            vin[i].position = toScreen(insidePts[i]);
            vin[i].color    = sf::Color(80, 230, 120);
        }
        window.draw(vout);
        window.draw(vin);

        // Дуга и рамка
        window.draw(arc);
        window.draw(square);

        // Крупно π внизу по центру
        std::ostringstream p;
        p << "pi ~ " << pi;
        big.setString(p.str());
        big.setFillColor(sf::Color(255, 230, 60));
        sf::FloatRect bb = big.getLocalBounds();
        big.setOrigin(bb.left + bb.width / 2.f, bb.top + bb.height / 2.f);
        big.setPosition(W / 2.f, H - 40.f);
        window.draw(big);

        window.draw(info);
        window.display();
    }

    return 0;
}