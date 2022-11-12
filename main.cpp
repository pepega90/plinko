#include <raylib.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>

const int WIDTH = 480;
const int HEIGHT = 640;

struct Ball
{
    Vector2 pos;
    Vector2 vel;
    Vector2 acc;
    Vector2 sumForce;

    float radius;
    float mass;
    float invMass;
    float restituion;

    Ball(float x, float y, float radius, float mass)
    {
        this->pos.x = x;
        this->pos.y = y;
        this->radius = radius;
        this->mass = mass;
        this->restituion = 1.0;
        if (this->mass != 0.0)
        {
            this->invMass = 1.0 / mass;
        }
        else
        {
            this->invMass = 0.0;
        }
    }

    bool IsStatic()
    {
        const float epsilon = 0.005f;
        return fabs(invMass - 0.0) < epsilon;
    }

    void applyImpulse(Vector2 j)
    {
        if (IsStatic())
            return;

        vel += j * invMass;
    }

    void applyForce(Vector2 f)
    {
        sumForce += f;
    }

    void ClearForce()
    {
        sumForce = Vector2();
    }

    void Update(float &dt)
    {
        if (IsStatic())
            return;
        acc = sumForce / invMass;
        vel += acc * dt;
        pos += vel * dt;

        ClearForce();
    }
};

struct Contact
{
    Ball *a;
    Ball *b;

    Vector2 start;
    Vector2 end;

    Vector2 normal;
    float depth;

    Contact() = default;
    ~Contact() = default;

    // projection method
    void ResolvePenetration();
    // impulse method
    void ResolveCollision();
};

void Contact::ResolvePenetration()
{
    // jika mass bernilai 0.0, itu akan menjadi static Ball
    // disini saya koment saja
    if (a->IsStatic() && b->IsStatic())
    {
        return;
    }

    float da = depth / (a->invMass + b->invMass) * a->invMass;
    float db = depth / (a->invMass + b->invMass) * b->invMass;

    a->pos -= normal * da;
    b->pos += normal * db;
}

void Contact::ResolveCollision()
{
    ResolvePenetration();

    // cari restitusi yang paling kecil
    float e = std::min(a->restituion, b->restituion);

    // kalkulasi relative velocity
    Vector2 vrel = (a->vel - b->vel);

    // projection relative veloticy ke surface normal
    float vRelDotNormal = vrel.Dot(normal);

    // kalkulasi impulse
    Vector2 impulseDirection = normal;
    float impulseMagnitude = -(1 + e) * vRelDotNormal / (a->invMass + b->invMass);

    // hasil impulse
    Vector2 j = impulseDirection * impulseMagnitude;

    // apply impulse
    a->applyImpulse(j);
    b->applyImpulse(-j);
}

struct Prize
{
    int prize;
    Vector2 pos;
    Rectangle rect;
    bool collide = false;
    Prize(int prize, float x, float y)
    {
        this->pos = Vector2(x, y);
        this->prize = prize;
    }
    void Update()
    {
        rect.x = pos.x;
        rect.y = pos.y;
        rect.width = 20;
        rect.height = 20;
        // DrawRectangleRec(rect, WHITE);
        DrawText(std::to_string(prize).c_str(), pos.x - 10, pos.y, 20, YELLOW);
    }

    ~Prize() = default;
};

bool IsCollide(Ball *a, Ball *b, Contact &contact)
{

    const Vector2 ab = b->pos - a->pos;
    const float radiusSum = a->radius + b->radius;

    bool isColliding = ab.MagnitudeSquared() <= (radiusSum * radiusSum);

    if (!isColliding)
    {
        return false;
    }

    contact.a = a;
    contact.b = b;

    contact.normal = ab;
    contact.normal.Normalize();

    contact.start = b->pos - contact.normal * b->radius;
    contact.end = a->pos + contact.normal * a->radius;

    contact.depth = (contact.end - contact.start).Magnitude();

    return true;
}

void rectCollide(Rectangle &rect, Ball *b)
{
    float nearX = std::max(rect.x, std::min(b->pos.x, rect.x + rect.width));
    float nearY = std::max(rect.y, std::min(b->pos.y, rect.y + rect.height));

    Vector2 dist = Vector2(b->pos.x - nearX, b->pos.y - nearY);

    if (b->vel.Dot(dist) < 0)
    {
        Vector2 dNormal = Vector2(-dist.y, dist.x);
        float normalAngle = std::atan2(dNormal.y, dNormal.x);
        float incomingAngle = std::atan2(b->vel.y, b->vel.x);
        float theta = normalAngle - incomingAngle;
        b->vel = b->vel.Rotate(2 * theta);
    }

    float depth = b->radius - dist.Magnitude();
    Vector2 depthVector = dist.Normalize();
    DrawLine(b->pos.x, b->pos.y, b->pos.x + depthVector.x * 15, b->pos.y + depthVector.y * 15, RED);
    depthVector *= depth;
    b->applyImpulse(depthVector);
}

int main()
{
    InitWindow(WIDTH, HEIGHT, "Plinko");
    SetTargetFPS(60);

    // list of balls
    int row = 6;
    int col = 7;

    std::vector<Ball *> balls;
    for (int i = 0; i < row; i++)
    {
        if (i % 2 == 0)
            col = 7;
        else
            col = col - 1;
        for (int j = 0; j < col; j++)
        {
            float x;
            if (i % 2 == 0)
            {
                x = 45 + j * 65;
            }
            else
            {
                x = 80 + j * 65;
            }
            float y = 100 + i * 80;
            Ball *b = new Ball(x, y, 5, 0);
            balls.push_back(b);
        }
    }

    // score
    std::vector<Rectangle> pipes;
    for (int i = 0; i < 1; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            Rectangle rect;
            rect.x = 10 + j * 65;
            rect.y = HEIGHT - 80 + i * 80;
            rect.width = 5;
            rect.height = 80;
            pipes.push_back(rect);
        }
    }
    Rectangle p1 = Rectangle{0, 70, 20, 30};
    Rectangle p2 = Rectangle{WIDTH - 20, 70, 20, 30};
    Rectangle p3 = Rectangle{WIDTH - 20, 230, 20, 30};
    Rectangle p4 = Rectangle{0, 230, 20, 30};
    Rectangle p5 = Rectangle{0, 390, 20, 30};
    Rectangle p6 = Rectangle{WIDTH - 20, 390, 20, 30};
    pipes.push_back(p1);
    pipes.push_back(p2);
    pipes.push_back(p3);
    pipes.push_back(p4);
    pipes.push_back(p5);
    pipes.push_back(p6);

    Vector2 mousePos;

    Color listColor[] = {YELLOW, ORANGE, PINK, RED, BLUE, PURPLE, GREEN};
    int randomVal = 0;

    // prize
    std::vector<Prize *> prizes;
    int listPrize[] = {100, 500, 0, 1000, 0, 500, 100};
    for (int i = 0; i < 7; i++)
    {
        Prize *p = new Prize(listPrize[i], 30 + i * 66, HEIGHT - 40);
        prizes.push_back(p);
    }

    // money
    int money = 500;
    Prize *collidePrize = nullptr;

    while (!WindowShouldClose())
    {

        ClearBackground(BLACK);
        BeginDrawing();

        float dt = GetFrameTime();
        mousePos = GetMousePosition();

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mousePos.y < 100 && balls.size() < 40)
        {
            Ball *b = new Ball(mousePos.x, mousePos.y, 25, 1.);
            b->restituion = 0.4;
            randomVal = GetRandomValue(0, 6);
            money -= 100;
            balls.push_back(b);
        }

        if (IsKeyPressed(KEY_R))
        {
            balls.erase(balls.begin() + balls.size() - 1);
        }

        // applying force
        for (auto &b : balls)
        {
            Vector2 grav = Vector2(0.0, 980);
            b->applyForce(grav);
        }
        // update balls
        for (auto &b : balls)
        {
            b->Update(dt);
        }

        // bikin bola tetap stay di layar
        for (auto &balls : balls)
        {
            if (balls->pos.x - balls->radius <= 0)
            {
                balls->pos.x = balls->radius;
                balls->vel.x *= -0.9;
            }
            else if (balls->pos.x + balls->radius >= GetScreenWidth())
            {
                balls->pos.x = GetScreenWidth() - balls->radius;
                balls->vel.x *= -0.9;
            }
            // if (balls->pos.y - balls->radius <= 0)
            // {
            //     balls->pos.y = balls->radius;
            //     balls->vel.y *= -0.9;
            // }
            // else if (balls->pos.y + balls->radius >= GetScreenHeight())
            // {
            //     balls->pos.y = GetScreenHeight() - balls->radius;
            //     balls->vel.y *= -0.9;
            // }
        }

        // resolve ball collision
        for (int i = 0; i <= balls.size() - 1; i++)
        {
            for (int j = i + 1; j < balls.size(); j++)
            {
                Ball *a = balls[i];
                Ball *b = balls[j];
                Contact contact;
                if (IsCollide(a, b, contact))
                {
                    contact.ResolveCollision();
                }
            }
        }

        // remove bola jika sudah lebih dari batas layar
        auto removedBall = std::remove_if(balls.begin(), balls.end(), [&](Ball *b)
                                          { return b->pos.y > HEIGHT; });

        if (removedBall != balls.end())
        {
            balls.erase(removedBall);
            collidePrize->collide = false;
            collidePrize = nullptr;
        }

        // check collision ball with pipes
        for (int i = 0; i < balls.size(); i++)
        {
            for (int j = 0; j < pipes.size(); j++)
            {
                if (CheckCollisionCircleRec(balls[i]->pos, balls[i]->radius, pipes[j]))
                {
                    rectCollide(pipes[j], balls[i]);
                }
            }
        }

        // check collision dengan prizes
        for (int i = 0; i < balls.size(); i++)
        {
            for (int j = 0; j < prizes.size(); j++)
            {
                if (CheckCollisionCircleRec(balls[i]->pos, balls[i]->radius, prizes[j]->rect) && !prizes[j]->collide)
                {
                    money += prizes[j]->prize;
                    prizes[j]->collide = true;
                    collidePrize = prizes[j];
                }
            }
        }

        // draw prizes
        for (auto &p : prizes)
        {
            p->Update();
        }

        // draw pipes
        for (auto &p : pipes)
        {
            DrawRectangleRec(p, WHITE);
        }

        // draw balls
        for (auto &b : balls)
        {
            if (b->IsStatic())
                DrawCircleLines(b->pos.x, b->pos.y, b->radius, WHITE);
            else
                DrawCircleV(b->pos, b->radius, listColor[randomVal]);
        }

        // menampilkan posisi mouse untuk debug
        // std::string mousePosText = "x = " + std::to_string(GetMousePosition().x) + ", y = " + std::to_string(GetMousePosition().y);
        // DrawText(mousePosText.c_str(), GetMousePosition().x, GetMousePosition().y, 20, RED);

        // draw money
        std::string moneyText = "Money: " + std::to_string(money);
        DrawText(moneyText.c_str(), 5, 5, 20, WHITE);
        EndDrawing();
    }
    CloseWindow();

    return 0;
}