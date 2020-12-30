#include "Player.h"

#include <algorithm>

#include "core/Game.h"
#include "core/constants.h"
#include "draw/Animation.h"
#include "draw/draw.h"
#include "util/rectangle.h"

//TODO make time checking more general

Player::Player() :
    PositionalObject((LEVEL_WIDTH - TILE_DIMENSIONS) / 2, LEVEL_HEIGHT - VIEW_MAX_PLAYER_DISTANCE),
    KineticObject(),
    DirectionalObject(),
    ColliderObject(COLLIDER),
    StateObject(State::Air) {}

void Player::update()
{
    float lastX = x;
    float lastY = y;
    float lastVelocityY = velocity.y;

    if (IsKeyUp(KEY_K))
        jumpBoostEndTime.end();

    if (isState(State::Air) && IsKeyPressed(KEY_K))
        jumpBuffer.start();

    if (hitStunTime.isExpired())
    {
        if (IsKeyDown(KEY_A) && velocity.x > 0 || IsKeyDown(KEY_D) && velocity.x < 0)
        {
            turnTime.start();
            velocity.x = 0;
        }

        if (turnTime.isExpired())
            velocity.x += ACCELERATION * Game::delta() * (IsKeyDown(KEY_D) - IsKeyDown(KEY_A));

        if (((velocity.y == 0 && lastVelocityY >= 0) ||
            coyoteTime.isOngoing()) &&
            (IsKeyPressed(KEY_K) ||
                (jumpBuffer.isOngoing()) && IsKeyDown(KEY_K)))
        {
            velocity.y = JUMP_SPEED;
            coyoteTime.end();
            jumpBuffer.end();
            jumpBoostStartTime.start();
            jumpBoostEndTime.start();
            setState(State::Air);
        }
        else if (velocity.y < 0 && IsKeyDown(KEY_K) && jumpBoostStartTime.isExpired() && jumpBoostEndTime.isOngoing())
        {
            velocity.y = JUMP_SPEED;
        }
    }

    looksLeft = hitStunTime.isOngoing() ? looksLeft : IsKeyDown(KEY_A) ? true : IsKeyDown(KEY_D) ? false : looksLeft;

    velocity.y += GRAVITY * Game::delta();
    velocity *= 1.0f - DRAG * Game::delta();
    
    if (!isState(State::Air) || (hitStunTime.isExpired() && (IsKeyDown(KEY_A) || IsKeyDown(KEY_D))))
        velocity *= 1.0f - FRICTION * Game::delta();

    if (abs(velocity.x) < SPEED_THRESHOLD)
        velocity.x = 0;

    applyVelocity();

    Level& level = Game::getLevel();

    if (collisionEnabled ?
        level.handleCollision(position, COLLIDER, velocity).y < 0 :
        (level.handleWallsCollision(position, COLLIDER, velocity), false))
    {
        coyoteTime.start();

        if (velocity.x == 0)
            setState(State::Idle);
        else
            setState(State::Walk);
    }
    else
    {
        setState(State::Air);
    }

    if (!collisionEnabled && powerupTime.isExpired() && !level.collides(getCollider()))
        collisionEnabled = true;
}

void Player::draw()
{
    for (int i = 0; i < health; ++i)
        DrawSpriteScreen(Sprite::get("spr_PlayerLife"), i * TILE_DIMENSIONS, 0);

    if (hitStunTime.isOngoing())
        DrawSpriteWorld(Sprite::get("spr_PlayerHurt"), position, looksLeft);
    else
        switch (getState())
        {
        case State::Idle:
            DrawSpriteWorld(Animation::get("anim_PlayerIdle").getCurrentSprite(getStateTime().elapsed(), true), position, looksLeft);
            break;
        case State::Walk:
            DrawSpriteWorld(Animation::get("anim_PlayerWalk").getCurrentSprite(getStateTime().elapsed(), true), position, looksLeft);
            break;
        case State::Air:
            if (velocity.y <= SLOW_JUMP_THRESHOLD)
                DrawSpriteWorld(Sprite::get("spr_PlayerFastJump"), position, looksLeft);
            else if (velocity.y <= JUMP_HOVER_THRESHOLD) //velocity.y < PLAYER_FALL_HOVER_THRESHOLD && velocity.y > PLAYER_JUMP_HOVER_THRESHOLD)
                DrawSpriteWorld(Sprite::get("spr_PlayerSlowJump"), position, looksLeft);
            else if (velocity.y <= FALL_HOVER_THRESHOLD)
                DrawSpriteWorld(Sprite::get("spr_PlayerHover"), position, looksLeft);
            else
                DrawSpriteWorld(Sprite::get("spr_PlayerFall"), position, looksLeft);
            break;
        }

    if (!collisionEnabled)
        DrawSpriteWorld(Sprite::get("spr_PlayerAura"), position - HALF_TILE_DIMENSIONS, false, true);
}

void Player::damage(Vector2 knockback, float hitStun)
{
    if (!isInvulnerable())
    {
        --health;
        invulnerabilityTime.start();
        hitStunTime.start(hitStun);

        velocity = knockback;
    }
}

void Player::powerup()
{
    collisionEnabled = false;
    powerupTime.start();
}

bool Player::isInvulnerable()
{
    return invulnerabilityTime.isOngoing();
}