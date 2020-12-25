#pragma once

#include <raylib.h>

#include "Sprite.h"

class View
{
private:
    float y;
    float scrollSpeed;

public:
    View();

    void drawSprite(const Sprite& sprite, Vector2 position);
    void drawSprite(const Sprite& sprite, int x, int y);
    void update(float t);

    float getY();
};