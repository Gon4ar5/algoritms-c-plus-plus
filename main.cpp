#include <SFML/Graphics.hpp>
#include <random>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>

// Наша собственная точка — без sf::
struct Point {
    float x, y;
};

int main() {
    const unsigned W = 900, H = 900;
    sf::RenderWindow window(sf::VideoMode(W, H), "Monte-Carlo Pi");
    window.setFramerateLimit(60);

    const float margin = 60.f;
    const float size   = std::min(W, H) - 2 * margin;
    const float ox     = (W - size) / 2.f;
    const float oy     = (H - size) / 2.f;

    // Из [0,1]x[0,1] -> экранные координаты (y вверх). Возвращает наш Point.
    auto toScreen = [&](float x, float y) -> Point {
        return { ox + x * size, oy + (1.f - y) * size };
    };

    // Рамка квадрата — тут SFML сам хочет Vector2f, но это одна строчка
    sf::RectangleShape square(sf::Vector2f(size, size));
    square.setPosition(ox, oy);
    square.setFillColor(sf::Color::Transparent);
    square.setOutlineColor(sf::Color::White);
    square.setOutlineThickness(2.f);

    // Дуга четверти круга
    const int ARC_SEG = 400;
    sf::VertexArray arc(sf::LineStrip, ARC_SEG + 1);
    for (int i = 0; i <= ARC_SEG; ++i) {
        float t   = (float)i / ARC_SEG;
        float ang = t * 3.14159265358979f / 2.f;
        Point p = toScreen(std::cos(ang), std::sin(ang));
        arc[i].position = sf::Vector2f(p.x, p.y);   // <-- перевод здесь
        arc[i].color    = sf::Color(120, 200, 255);
    }

    // Точки — храним как наши Point, а не sf::Vector2f
    std::vector<Point> insidePts, outsidePts;

    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(0.f, 1.f);

    long long total = 0, inside = 0;

    sf::Font font;
    font.loadFromFile("/System/Library/Fonts/Supplemental/Arial.ttf");

    sf::Text text;
    text.setFont(font);
    text.setCharacterSize(22);
    text.setFillColor(sf::Color::White);
    text.setPosition(15.f, 15.f);

    const int       pointsPerFrame = 300;
    const long long maxPoints      = 500000;

    while (window.isOpen()) {
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) window.close();
            if (e.type == sf::Event::KeyPressed &&
                e.key.code == sf::Keyboard::Escape) window.close();
        }

        if (total < maxPoints) {
            for (int i = 0; i < pointsPerFrame && total < maxPoints; ++i) {
                float x = dist(rng);
                float y = dist(rng);
                bool in = (x * x + y * y) <= 1.f;
                Point p = toScreen(x, y);
                if (in) { ++inside; insidePts.push_back(p); }
                else    { outsidePts.push_back(p); }
                ++total;
            }
        }

        double pi = total > 0 ? 4.0 * (double)inside / (double)total : 0.0;

        std::ostringstream ss;
        ss << "Monte-Carlo Pi  |  N=" << total
           << "  inside=" << inside
           << "  pi ~ " << pi;
        window.setTitle(ss.str());

        std::ostringstream t;
        t << "N = " << total << "\n"
          << "inside = " << inside << "\n"
          << "pi ~ " << pi;
        text.setString(t.str());

        // ---- Рисование ----
        window.clear(sf::Color(20, 20, 30));

        // В SFML-вершинах всё равно нужен .position типа sf::Vector2f,
        // поэтому аккуратно переводим наш Point -> sf::Vector2f.
        sf::VertexArray vout(sf::Points, outsidePts.size());
        for (size_t i = 0; i < outsidePts.size(); ++i) {
            vout[i].position = sf::Vector2f(outsidePts[i].x, outsidePts[i].y);
            vout[i].color    = sf::Color(230, 80, 80);
        }
        sf::VertexArray vin(sf::Points, insidePts.size());
        for (size_t i = 0; i < insidePts.size(); ++i) {
            vin[i].position = sf::Vector2f(insidePts[i].x, insidePts[i].y);
            vin[i].color    = sf::Color(80, 230, 120);
        }
        window.draw(vout);
        window.draw(vin);

        window.draw(arc);
        window.draw(square);
        window.draw(text);

        window.display();
    }

    return 0;
}